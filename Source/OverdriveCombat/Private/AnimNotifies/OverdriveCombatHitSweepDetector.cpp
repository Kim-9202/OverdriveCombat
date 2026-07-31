// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatHitSweepDetector.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "PrimitiveDrawingUtils.h"
#endif

#define LOCTEXT_NAMESPACE "OverdriveCombatHitSweepDetector"

#if WITH_EDITOR
namespace
{
	/** 궤적선 굵기. 촘촘한 셰이프 와이어 사이에서 경로가 묻히지 않을 정도. */
	constexpr float SweepPathLineThickness = 2.0f;
}
#endif

FCollisionShape UOverdriveCombatHitSweepDetector::MakeCollisionShape() const
{
	switch (ShapeType)
	{
	case EOverdriveCombatSweepShapeType::Capsule:
		return FCollisionShape::MakeCapsule(Radius, HalfHeight);

	case EOverdriveCombatSweepShapeType::Box:
		return FCollisionShape::MakeBox(BoxExtent);

	case EOverdriveCombatSweepShapeType::Sphere:
	default:
		return FCollisionShape::MakeSphere(Radius);
	}
}

FTransform UOverdriveCombatHitSweepDetector::ComposeWorldPlacement(const FTransform& SampleCompSpace, const FTransform& ComponentToWorld) const
{
	// Child * Parent: 오프셋(소켓 로컬) -> 소켓(컴포넌트 공간) -> 컴포넌트(월드).
	return RelativeTransform * SampleCompSpace * ComponentToWorld;
}

void UOverdriveCombatHitSweepDetector::SetRelativeTransform(const FTransform& InTransform)
{
	RelativeTransform = InTransform;

	// 스케일은 판정에 쓰지 않는다. 기즈모 / 수기 입력 잔여값이 남지 않도록 항상 1로 고정한다.
	RelativeTransform.SetScale3D(FVector::OneVector);
}

void UOverdriveCombatHitSweepDetector::CommitTickHits(const TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& TickBestHits, TSet<TWeakObjectPtr<UPrimitiveComponent>>& InOutAlreadyHitComponents, FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& Pair : TickBestHits)
	{
		// 확정된 컴포넌트는 다음 틱 스킵셋에 반영한다(크로스틱 중복 방지).
		InOutAlreadyHitComponents.Add(Pair.Key);

		// raw new 를 넘기는 것이 정상 API 다. 핸들이 TSharedPtr 로 감싸 소유권을 가져간다. delete 금지.
		OutTargetData.Add(new FOverdriveCombatTargetData_AttackHit(Pair.Value, AttackTypeTag));
	}
}

void UOverdriveCombatHitSweepDetector::DetectHitForSegment(const USkeletalMeshComponent* MeshComp, const AActor* OwnerActor, const FTransform& StartSampleCompSpace, const FTransform& EndSampleCompSpace, const FTransform& ComponentToWorld, const FVector& WorldOrigin, const TSet<TWeakObjectPtr<UPrimitiveComponent>>& AlreadyHitComponents, TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& InOutTickBestHits) const
{
	if (MeshComp == nullptr || OwnerActor == nullptr)
	{
		return;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);

	// 공격 판정에 트라이앵글 메시 정밀도는 필요 없다. 단순 콜리전으로만 검사한다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverdriveCombatSweep), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors);
	QueryParams.AddIgnoredActors(AttachedActors);

	const FCollisionShape Shape = MakeCollisionShape();

	const FTransform StartXform = ComposeWorldPlacement(StartSampleCompSpace, ComponentToWorld);
	const FTransform EndXform = ComposeWorldPlacement(EndSampleCompSpace, ComponentToWorld);

	const FVector Start = StartXform.GetLocation();
	const FVector End = EndXform.GetLocation();
	const FQuat Rotation = StartXform.GetRotation();

	TArray<FHitResult> SweepHits;

	// 오버랩과 달리 FHitResult 를 돌려주므로 TargetData 를 만들 수 있다.
	World->SweepMultiByChannel(SweepHits, Start, End, Rotation, CollisionChannel, Shape, QueryParams);

#if ENABLE_DRAW_DEBUG
	OverdriveCombatDebug::DrawShapeSweep(World, Shape, Start, End, Rotation, SweepHits);
#endif

	// 이전 틱에서 이미 확정된 컴포넌트는 건너뛰고, 나머지는 이번 틱 최적맵에 원점 최근접으로 누적한다.
	for (const FHitResult& Hit : SweepHits)
	{
		if (AlreadyHitComponents.Contains(Hit.GetComponent()))
		{
			continue;
		}

		OverdriveCombatHitDetection::ConsiderClosestToOrigin(InOutTickBestHits, Hit, WorldOrigin);
	}
}

#if ENABLE_DRAW_DEBUG
void UOverdriveCombatHitSweepDetector::DrawDebugSweepSegment(const UWorld* World, const FTransform& StartSampleCompSpace, const FTransform& EndSampleCompSpace, const FTransform& ComponentToWorld) const
{
	if (World == nullptr)
	{
		return;
	}

	const FTransform StartXform = ComposeWorldPlacement(StartSampleCompSpace, ComponentToWorld);
	const FTransform EndXform = ComposeWorldPlacement(EndSampleCompSpace, ComponentToWorld);

	// 물리 판정 없이 부르는 경로(프리뷰)도 있으므로 히트는 호출자가 아니라 여기서 비워 둔다.
	const TArray<FHitResult> NoHits;

	OverdriveCombatDebug::DrawShapeSweep(World, MakeCollisionShape(), StartXform.GetLocation(), EndXform.GetLocation(), StartXform.GetRotation(), NoHits);
}
#endif

#if WITH_EDITOR
FName UOverdriveCombatHitSweepDetector::GetRelativeTransformPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatHitSweepDetector, RelativeTransform);
}

void UOverdriveCombatHitSweepDetector::DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const
{
	if (PDI == nullptr || MeshComp == nullptr)
	{
		return;
	}

	// 프리뷰 포즈의 소켓(컴포넌트 공간)에 오프셋을 얹어 월드 배치한다.
	const FTransform SocketCompSpace = MeshComp->GetSocketTransform(SocketName, RTS_Component);
	const FTransform WorldXform = ComposeWorldPlacement(SocketCompSpace, MeshComp->GetComponentTransform());

	OverdriveCombatDebug::DrawEditorShapeAt(PDI, MakeCollisionShape(), WorldXform.GetLocation(), WorldXform.GetRotation(), Color);
}

void UOverdriveCombatHitSweepDetector::DrawEditorSweepPath(FPrimitiveDrawInterface* PDI, const TArray<FTransform>& SamplesCompSpace, const FTransform& ComponentToWorld, const FLinearColor& ShapeColor, const FLinearColor& PathColor) const
{
	if (PDI == nullptr || SamplesCompSpace.Num() == 0)
	{
		return;
	}

	// 디텍터 셰이프는 샘플마다 바뀌지 않으므로 루프 밖에서 한 번만 만든다.
	const FCollisionShape Shape = MakeCollisionShape();

	FVector PreviousLocation = FVector::ZeroVector;
	FQuat PreviousRotation = FQuat::Identity;

	for (int32 Index = 0; Index < SamplesCompSpace.Num(); ++Index)
	{
		const FTransform WorldXform = ComposeWorldPlacement(SamplesCompSpace[Index], ComponentToWorld);
		const FVector Location = WorldXform.GetLocation();

		if (Index > 0)
		{
			// 세그먼트 스윕 볼륨. 회전은 시작 샘플 기준이라 DetectHitForSegment 가 실제로 스윕하는 볼륨과 같다.
			OverdriveCombatDebug::DrawEditorShapeSweep(PDI, Shape, PreviousLocation, Location, PreviousRotation, ShapeColor);

			// 앵커 중심을 잇는 궤적선. 셰이프 와이어에 묻히지 않도록 전면 레이어에 굵게 그린다.
			PDI->DrawLine(PreviousLocation, Location, PathColor, SDPG_Foreground, SweepPathLineThickness);
		}

		PreviousLocation = Location;
		PreviousRotation = WorldXform.GetRotation();
	}
}

EDataValidationResult UOverdriveCombatHitSweepDetector::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!AttackTypeTag.IsValid())
	{
		// 태그 없이도 판정 자체는 동작하므로 에러가 아니라 경고만 남긴다.
		Context.AddWarning(LOCTEXT("MissingAttackTypeTag", "디텍터에 AttackTypeTag 가 지정되지 않았습니다. 수신 어빌리티가 공격 타입을 구분할 수 없습니다."));
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
