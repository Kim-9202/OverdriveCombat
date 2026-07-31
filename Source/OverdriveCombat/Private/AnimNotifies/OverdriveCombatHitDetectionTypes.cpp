// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"
#include "OverdriveCombatTags.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"

#if ENABLE_DRAW_DEBUG
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "KismetTraceUtils.h"
#endif

#if WITH_EDITOR
// 와이어 헬퍼(DrawWireSphere 등)와 FPrimitiveDrawInterface 를 함께 끌어온다.
#include "PrimitiveDrawingUtils.h"
#endif

namespace
{
	/**
	 * Spec.Mode 에 따라 새 ImpactNormal 을 계산한다.
	 * 방향이 0 길이로 무너지면 히트가 준 원본 노멀을 유지한다.
	 */
	FVector ComputeImpactNormal(const FOverdriveCombatImpactNormalSpec& Spec, const FOverdriveCombatImpactContext& Context, const FHitResult& Hit)
	{
		switch (Spec.Mode)
		{
		case EOverdriveCombatImpactNormalMode::FromOrigin:
		{
			// Origin 에서 히트 지점으로 뻗는 바깥 방향.
			FVector Direction = Hit.ImpactPoint - Context.WorldOrigin;
			if (Direction.Normalize())
			{
				return Direction;
			}

			return Hit.ImpactNormal;
		}

		case EOverdriveCombatImpactNormalMode::TowardOrigin:
		{
			// 히트 지점에서 Origin 으로 향하는 방향.
			FVector Direction = Context.WorldOrigin - Hit.ImpactPoint;
			if (Direction.Normalize())
			{
				return Direction;
			}

			return Hit.ImpactNormal;
		}

		case EOverdriveCombatImpactNormalMode::FixedDirection:
		{
			// 컴포넌트 공간 방향을 월드로 회전(스케일 무시)한다.
			FVector WorldDirection = Context.ComponentToWorld.TransformVectorNoScale(Spec.Direction);
			if (WorldDirection.Normalize())
			{
				return WorldDirection;
			}

			return Hit.ImpactNormal;
		}

		default:
			return Hit.ImpactNormal;
		}
	}
}

namespace OverdriveCombatHitEvents
{
	void SendHitEvent(AActor* InstigatorActor, const FGameplayAbilityTargetDataHandle& TargetData, FGameplayTag EventTag)
	{
		if (InstigatorActor == nullptr || TargetData.Num() == 0)
		{
			return;
		}

		// 디텍터는 FHitResult 를 가진 TargetData 만 만들지만, 다른 호출자를 대비해 방어적으로 확인한다.
		const FHitResult* FirstHit = TargetData.Get(0)->GetHitResult();
		if (FirstHit == nullptr)
		{
			return;
		}

		// 태그가 비어 있으면 기본 히트 이벤트 태그로 대체한다.
		if (!EventTag.IsValid())
		{
			EventTag = OverdriveCombatTags::Combat_Event_Hit;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = InstigatorActor;

		// 다중 히트의 진실은 TargetData 에 있다. Target 은 단일 히트일 때만 의미가 있다.
		Payload.Target = (TargetData.Num() == 1) ? FirstHit->GetActor() : InstigatorActor;
		Payload.TargetData = TargetData;

		if (UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorActor))
		{
			Payload.InstigatorTags = InstigatorASC->GetOwnedGameplayTags();

			// 수신 어빌리티가 데미지 GE 로 그대로 흘려보낼 수 있도록 이펙트 컨텍스트를 미리 채운다.
			FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
			EffectContext.AddInstigator(InstigatorActor, InstigatorActor);
			EffectContext.AddHitResult(*FirstHit);

			Payload.ContextHandle = EffectContext;
		}

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InstigatorActor, EventTag, Payload);
	}

	void ApplyImpactPostProcess(FGameplayAbilityTargetDataHandle& TargetData, const FOverdriveCombatImpactContext& Context, const FOverdriveCombatImpactNormalSpec& Spec)
	{
		for (int32 Index = 0; Index < TargetData.Num(); ++Index)
		{
			FGameplayAbilityTargetData* Data = TargetData.Get(Index);
			if (Data == nullptr || Data->GetScriptStruct() != FOverdriveCombatTargetData_AttackHit::StaticStruct())
			{
				continue;
			}

			// 우리 타입임을 GetScriptStruct 로 확인했으므로 정적 캐스트가 안전하다.
			FOverdriveCombatTargetData_AttackHit* Typed = static_cast<FOverdriveCombatTargetData_AttackHit*>(Data);
			Typed->Origin = Context.WorldOrigin;

			if (Spec.Mode != EOverdriveCombatImpactNormalMode::None)
			{
				const FVector NewNormal = ComputeImpactNormal(Spec, Context, Typed->HitResult);
				Typed->HitResult.ImpactNormal = NewNormal;
				Typed->HitResult.Normal = NewNormal;
			}
		}
	}
}

namespace OverdriveCombatHitDetection
{
	void ConsiderClosestToOrigin(TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& BestByComponent, const FHitResult& Hit, const FVector& WorldOrigin)
	{
		const TWeakObjectPtr<UPrimitiveComponent> Component = Hit.GetComponent();

		FHitResult* Existing = BestByComponent.Find(Component);
		if (Existing == nullptr)
		{
			BestByComponent.Add(Component, Hit);
			return;
		}

		// sqrt 는 필요 없다. 제곱거리로 원점 최근접을 비교한다.
		const double NewDistSq = FVector::DistSquared(Hit.ImpactPoint, WorldOrigin);
		const double ExistingDistSq = FVector::DistSquared(Existing->ImpactPoint, WorldOrigin);
		if (NewDistSq < ExistingDistSq)
		{
			*Existing = Hit;
		}
	}
}

#if ENABLE_DRAW_DEBUG

namespace OverdriveCombatDebug
{
	static TAutoConsoleVariable<int32> CVarDrawHitDetection(
		TEXT("od.Combat.DrawHitDetection"),
		0,
		TEXT("애님 노티파이의 히트 판정 셰이프를 그린다.\n")
		TEXT("0: 끔\n")
		TEXT("1: 한 프레임\n")
		TEXT("2: od.Combat.DrawHitDetectionDuration 동안 유지\n")
		TEXT("3: 지울 때까지 유지"),
		ECVF_Cheat);

	static TAutoConsoleVariable<float> CVarDrawHitDetectionDuration(
		TEXT("od.Combat.DrawHitDetectionDuration"),
		2.0f,
		TEXT("od.Combat.DrawHitDetection 이 2 일 때 디버그 셰이프를 유지할 시간(초)."),
		ECVF_Cheat);

	void DrawShapeSweep(const UWorld* World, const FCollisionShape& Shape, const FVector& Start, const FVector& End, const FQuat& Rotation, const TArray<FHitResult>& Hits, int32 OverrideDrawMode)
	{
		const int32 DrawMode = OverrideDrawMode > 0 ? OverrideDrawMode : CVarDrawHitDetection.GetValueOnAnyThread();

		if (World == nullptr || DrawMode <= 0)
		{
			return;
		}

		// CVar 모드(0~3)가 EDrawDebugTrace::Type(None/ForOneFrame/ForDuration/Persistent)과 값이 그대로 일치한다.
		const EDrawDebugTrace::Type DrawType = static_cast<EDrawDebugTrace::Type>(DrawMode);
		const float DrawTime = (DrawMode == 2) ? CVarDrawHitDetectionDuration.GetValueOnAnyThread() : 0.0f;
		const bool bHit = (Hits.Num() > 0);

		const FLinearColor TraceColor = FLinearColor::Green;
		const FLinearColor TraceHitColor = FLinearColor::Red;

		// 엔진 트레이스 디버그 헬퍼가 스윕 볼륨(구=캡슐, 박스=확장 박스)과 히트를 한 번에 그려 준다.
		if (Shape.IsSphere())
		{
			DrawDebugSphereTraceMulti(World, Start, End, Shape.GetSphereRadius(), DrawType, bHit, Hits, TraceColor, TraceHitColor, DrawTime);
		}
		else if (Shape.IsCapsule())
		{
			DrawDebugCapsuleTraceMulti(World, Start, End, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight(), Rotation.Rotator(), DrawType, bHit, Hits, TraceColor, TraceHitColor, DrawTime);
		}
		else if (Shape.IsBox())
		{
			DrawDebugBoxTraceMulti(World, Start, End, Shape.GetExtent(), Rotation.Rotator(), DrawType, bHit, Hits, TraceColor, TraceHitColor, DrawTime);
		}
	}
}

#endif // ENABLE_DRAW_DEBUG

#if WITH_EDITOR

namespace OverdriveCombatDebug
{
	void DrawEditorShapeAt(FPrimitiveDrawInterface* PDI, const FCollisionShape& Shape, const FVector& Location, const FQuat& Rotation, const FLinearColor& Color)
	{
		const FVector AxisX = Rotation.GetAxisX();
		const FVector AxisY = Rotation.GetAxisY();
		const FVector AxisZ = Rotation.GetAxisZ();

		if (Shape.IsSphere())
		{
			DrawWireSphere(PDI, Location, Color, Shape.GetSphereRadius(), 16, SDPG_World);
		}
		else if (Shape.IsCapsule())
		{
			DrawWireCapsule(PDI, Location, AxisX, AxisY, AxisZ, Color, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight(), 16, SDPG_World);
		}
		else if (Shape.IsBox())
		{
			DrawOrientedWireBox(PDI, Location, AxisX, AxisY, AxisZ, Shape.GetExtent(), Color, SDPG_World);
		}
	}

	void DrawEditorShapeSweep(FPrimitiveDrawInterface* PDI, const FCollisionShape& Shape, const FVector& Start, const FVector& End, const FQuat& Rotation, const FLinearColor& Color)
	{
		// 0-길이 스윕은 방향 벡터가 없어 실루엣이 성립하지 않는다. 제자리 판정이므로 셰이프 하나로 그린다.
		if (End.Equals(Start))
		{
			DrawEditorShapeAt(PDI, Shape, Start, Rotation, Color);

			return;
		}

		const FVector TraceVec = End - Start;

		if (Shape.IsSphere())
		{
			// 구 스윕의 실제 판정 볼륨 = 스윕 방향으로 늘인 캡슐. DrawWireCapsule 의 HalfHeight 는
			// 캡 포함 전체 절반 높이라 DrawDebugCapsule 과 의미가 같다(내부에서 radius 차감).
			const FVector Center = Start + TraceVec * 0.5f;
			const double HalfHeight = TraceVec.Size() * 0.5 + Shape.GetSphereRadius();
			const FMatrix SweepMatrix = FRotationMatrix::MakeFromZ(TraceVec);
			DrawWireCapsule(PDI, Center, SweepMatrix.GetUnitAxis(EAxis::X), SweepMatrix.GetUnitAxis(EAxis::Y), SweepMatrix.GetUnitAxis(EAxis::Z), Color, Shape.GetSphereRadius(), HalfHeight, 16, SDPG_World);

			return;
		}

		DrawEditorShapeAt(PDI, Shape, Start, Rotation, Color);
		DrawEditorShapeAt(PDI, Shape, End, Rotation, Color);

		if (Shape.IsCapsule())
		{
			PDI->DrawLine(Start, End, Color, SDPG_World);

			return;
		}

		// 박스: 시작 박스의 꼭짓점 8개를 스윕 벡터만큼 연결한다.
		const FVector HalfSize = Shape.GetExtent();
		for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
		{
			const FVector SignedExtent(
				(CornerIndex & 1) ? HalfSize.X : -HalfSize.X,
				(CornerIndex & 2) ? HalfSize.Y : -HalfSize.Y,
				(CornerIndex & 4) ? HalfSize.Z : -HalfSize.Z);
			const FVector Corner = Start + Rotation.RotateVector(SignedExtent);
			PDI->DrawLine(Corner, Corner + TraceVec, Color, SDPG_World);
		}
	}
}

#endif // WITH_EDITOR
