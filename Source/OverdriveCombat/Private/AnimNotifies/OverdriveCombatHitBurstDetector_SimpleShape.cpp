// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatHitBurstDetector_SimpleShape.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

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

		// 길이 0 스윕(제자리 판정)은 헬퍼가 셰이프 하나로 폴백한다.
		OverdriveCombatDebug::DrawEditorShapeSweep(PDI, Shape, Start, End, ShapeRotation, Color);
	}
}
#endif
