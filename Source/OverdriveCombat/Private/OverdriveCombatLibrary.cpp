// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatLibrary.h"
#include "Components/OverdriveCombatComponent.h"
#include "GameplayAbilities/OverdriveCombatBarrier.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"


UOverdriveCombatBarrier* UOverdriveCombatLibrary::CreateBarrier(UOverdriveCombatComponent* TargetOCCombat, TSubclassOf<UOverdriveCombatBarrier> NewBarrierClass, const FGameplayEffectSpecHandle& InBarrierEffectSpec)
{
	// BP 노출 함수는 잘못된 입력으로 크래시(check)하지 않는다 — 경고(ensure) 후 nullptr 반환.
	if (!ensure(TargetOCCombat != nullptr) || !ensure(NewBarrierClass != nullptr) || !ensure(InBarrierEffectSpec.IsValid()))
	{
		return nullptr;
	}

	UAbilitySystemComponent* InASC = TargetOCCombat->GetLinkedAbilitySystem();
	if (!ensure(InASC != nullptr))
	{
		return nullptr;
	}

	FGameplayEffectSpec& InSpec = *InBarrierEffectSpec.Data.Get();

	if (InSpec.Def == nullptr || !InSpec.Def->CanApply(InASC->GetActiveGameplayEffects(), InSpec))
	{
		return nullptr;
	}

	UOverdriveCombatBarrier* NewBarrier = NewObject<UOverdriveCombatBarrier>(TargetOCCombat, NewBarrierClass);

	return NewBarrier->ApplyBarrier(TargetOCCombat, InSpec) ? NewBarrier : nullptr;
}

FGameplayTag UOverdriveCombatLibrary::GetAttackTypeTagFromTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, int32 Index)
{
	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Index);

	if (TargetData == nullptr)
	{
		return FGameplayTag();
	}

	const UScriptStruct* ScriptStruct = TargetData->GetScriptStruct();

	if (ScriptStruct == nullptr || !ScriptStruct->IsChildOf(FOverdriveCombatTargetData_AttackHit::StaticStruct()))
	{
		return FGameplayTag();
	}

	return static_cast<const FOverdriveCombatTargetData_AttackHit*>(TargetData)->AttackTypeTag;
}

void UOverdriveCombatLibrary::BreakAttackHitTargetData(const FOverdriveCombatTargetData_AttackHit& AttackHit, FHitResult& HitResult, FGameplayTag& AttackTypeTag, FVector& Origin)
{
	HitResult = AttackHit.HitResult;			// 베이스(FGameplayAbilityTargetData_SingleTargetHit)의 페이로드
	AttackTypeTag = AttackHit.AttackTypeTag;
	Origin = FVector(AttackHit.Origin);			// FVector_NetQuantize10 → FVector
}

FGameplayEffectContextHandle UOverdriveCombatLibrary::DuplicateEffectContextHandle(const FGameplayEffectContextHandle& InHandle)
{
	if (!InHandle.IsValid())
	{
		return FGameplayEffectContextHandle();
	}

	return InHandle.Duplicate();
}


