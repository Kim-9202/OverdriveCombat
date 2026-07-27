// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatHitBurstDetector_SimpleShape.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "PrimitiveDrawingUtils.h"
#endif

void UOverdriveCombatHitBurstDetector_SimpleShape::CollectHits(const FOverdriveCombatHitBurstDetectorContext& Context, TArray<FHitResult>& OutHits) const
{
	const UWorld* World = Context.MeshComp->GetWorld();
	const AActor* InstigatorActor = Context.OwnerActor;

	const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(GetTraceChannel());

	// 공격 판정에 트라이앵글 메시 정밀도는 필요 없다. 단순 콜리전으로만 검사한다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverdriveCombatSimpleShape), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(InstigatorActor);

	TArray<AActor*> AttachedActors;
	InstigatorActor->GetAttachedActors(AttachedActors);
	QueryParams.AddIgnoredActors(AttachedActors);

	// RelativeTransform x 메시 컴포넌트. 각 셰이프의 로컬 배치를 이 위에 올린다.
	const FTransform BaseTransform = CalculateWorldTransform(Context.MeshComp);

	// 여러 셰이프가 같은 컴포넌트를 중복 타격할 때, 가장 빠른 히트가 아니라 Origin 최근접 히트를 남긴다.
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult> BestByComponent;

	for (const FOverdriveCombatSimpleShapeEntry& Entry : Shapes)
	{
		const FCollisionShape Shape = MakeCollisionShape(Entry);
		const FVector Start = BaseTransform.TransformPosition(Entry.StartLocation);
		const FVector End = BaseTransform.TransformPosition(Entry.EndLocation);
		const FQuat ShapeRotation = BaseTransform.GetRotation() * Entry.Rotation.Quaternion();

		TArray<FHitResult> SweepHits;

		// 오버랩과 달리 FHitResult 를 돌려주므로 TargetData 를 만들 수 있다. Start == End 면 제자리 판정.
		World->SweepMultiByChannel(SweepHits, Start, End, ShapeRotation, CollisionChannel, Shape, QueryParams);

#if ENABLE_DRAW_DEBUG
		// 애님 에디터 프리뷰 월드에서도 그대로 그려진다. 프리뷰에서 이벤트를 보낼지는 노티파이가 결정한다.
		OverdriveCombatDebug::DrawShapeSweep(World, Shape, Start, End, ShapeRotation, SweepHits);
#endif

		for (const FHitResult& Hit : SweepHits)
		{
			OverdriveCombatHitDetection::ConsiderClosestToOrigin(BestByComponent, Hit, Context.WorldOrigin);
		}
	}

	BestByComponent.GenerateValueArray(OutHits);
}

FCollisionShape UOverdriveCombatHitBurstDetector_SimpleShape::MakeCollisionShape(const FOverdriveCombatSimpleShapeEntry& Entry) const
{
	switch (Entry.ShapeType)
	{
	case EOverdriveCombatShapeType::Capsule:
		return FCollisionShape::MakeCapsule(Entry.Radius, Entry.HalfHeight);

	case EOverdriveCombatShapeType::Box:
		return FCollisionShape::MakeBox(Entry.BoxExtent);

	case EOverdriveCombatShapeType::Sphere:
	default:
		return FCollisionShape::MakeSphere(Entry.Radius);
	}
}

#if ENABLE_DRAW_DEBUG
void UOverdriveCombatHitBurstDetector_SimpleShape::DrawDebugDetection(const FOverdriveCombatHitBurstDetectorContext& Context, int32 OverrideDrawMode) const
{
	const UWorld* World = (Context.MeshComp != nullptr) ? Context.MeshComp->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}

	const FTransform BaseTransform = CalculateWorldTransform(Context.MeshComp);
	const TArray<FHitResult> NoHits; // 물리 판정을 하지 않으므로 히트 없음. 스윕 볼륨만 그린다.

	for (const FOverdriveCombatSimpleShapeEntry& Entry : Shapes)
	{
		const FCollisionShape Shape = MakeCollisionShape(Entry);
		const FVector Start = BaseTransform.TransformPosition(Entry.StartLocation);
		const FVector End = BaseTransform.TransformPosition(Entry.EndLocation);
		const FQuat ShapeRotation = BaseTransform.GetRotation() * Entry.Rotation.Quaternion();

		OverdriveCombatDebug::DrawShapeSweep(World, Shape, Start, End, ShapeRotation, NoHits, OverrideDrawMode);
	}
}
#endif

#if WITH_EDITOR
namespace
{
	/** 콜리전 셰이프 종류에 맞는 와이어프레임을 한 위치에 그린다. */
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

	/**
	 * 스윕 볼륨을 런타임 디버그(KismetTraceUtils)와 같은 모양으로 그린다:
	 * 구 = 전 구간을 덮는 캡슐 하나(DrawDebugSweptSphere), 캡슐 = 양 끝 + 중심 연결선(DrawDebugCapsuleTraceMulti),
	 * 박스 = 양 끝 + 꼭짓점 8개 연결선(DrawDebugSweptBox). 뷰포트 와이어와 PIE 디버그가 같은 볼륨으로 읽힌다.
	 */
	void DrawEditorShapeSweep(FPrimitiveDrawInterface* PDI, const FCollisionShape& Shape, const FVector& Start, const FVector& End, const FQuat& Rotation, const FLinearColor& Color)
	{
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

void UOverdriveCombatHitBurstDetector_SimpleShape::DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const
{
	if (PDI == nullptr)
	{
		return;
	}

	const FTransform BaseTransform = CalculateWorldTransform(MeshComp);

	for (const FOverdriveCombatSimpleShapeEntry& Entry : Shapes)
	{
		const FCollisionShape Shape = MakeCollisionShape(Entry);
		const FVector Start = BaseTransform.TransformPosition(Entry.StartLocation);
		const FVector End = BaseTransform.TransformPosition(Entry.EndLocation);
		const FQuat ShapeRotation = BaseTransform.GetRotation() * Entry.Rotation.Quaternion();

		// 길이 0 스윕이면 제자리 판정 — 셰이프 하나만 그린다.
		if (End.Equals(Start))
		{
			DrawEditorShapeAt(PDI, Shape, Start, ShapeRotation, Color);
			continue;
		}

		DrawEditorShapeSweep(PDI, Shape, Start, End, ShapeRotation, Color);
	}
}
#endif
