// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatHitBurstDetector.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SkeletalMeshComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OverdriveCombatHitBurstDetector"

int32 UOverdriveCombatHitBurstDetector::DetectHit(const FOverdriveCombatHitBurstDetectorContext& Context, FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	// OwnerActor 는 트레이스 제외와 히트 필터의 기준이라 없으면 판정 자체가 성립하지 않는다.
	// CollectHits 의 역참조 가드도 겸한다.
	if (Context.MeshComp == nullptr || Context.OwnerActor == nullptr || Context.MeshComp->GetWorld() == nullptr)
	{
		return 0;
	}

	TArray<FHitResult> Hits;
	CollectHits(Context, Hits);

	PackTargetData(Hits, OutTargetData);

	return Hits.Num();
}

void UOverdriveCombatHitBurstDetector::SetRelativeTransform(const FTransform& InTransform)
{
	RelativeTransform = InTransform;

	// 스케일은 판정에 쓰지 않는다. 기즈모 / 수기 입력 잔여값이 남지 않도록 항상 1로 고정한다.
	RelativeTransform.SetScale3D(FVector::OneVector);
}

FTransform UOverdriveCombatHitBurstDetector::CalculateWorldTransform(const USkeletalMeshComponent* MeshComp) const
{
	if (MeshComp == nullptr)
	{
		return RelativeTransform;
	}

	// FTransform 합성은 Child * Parent 순서다. 뒤집으면 회전한 캐릭터에서 배치가 틀어진다.
	return RelativeTransform * MeshComp->GetComponentTransform();
}

void UOverdriveCombatHitBurstDetector::PackTargetData(const TArray<FHitResult>& Hits, FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	for (const FHitResult& Hit : Hits)
	{
		// raw new 를 넘기는 것이 정상 API 다. 핸들이 TSharedPtr 로 감싸 소유권을 가져간다.
		// 엔진 UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult 와 동일한 관용구. delete 금지.
		OutTargetData.Add(new FOverdriveCombatTargetData_AttackHit(Hit, AttackTypeTag));
	}
}

#if WITH_EDITOR
FName UOverdriveCombatHitBurstDetector::GetRelativeTransformPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatHitBurstDetector, RelativeTransform);
}

void UOverdriveCombatHitBurstDetector::DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const
{
	if (MeshComp == nullptr)
	{
		return;
	}

	FOverdriveCombatHitBurstDetectorContext Context;
	Context.MeshComp = MeshComp;
	Context.OwnerActor = MeshComp->GetOwner();

	DrawDebugDetection(Context, 1);
}

EDataValidationResult UOverdriveCombatHitBurstDetector::IsDataValid(FDataValidationContext& Context) const
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
