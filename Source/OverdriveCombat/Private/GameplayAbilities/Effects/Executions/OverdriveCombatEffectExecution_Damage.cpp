// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/Effects/Executions/OverdriveCombatEffectExecution_Damage.h"
#include "GameplayAbilities/OverdriveCombatAttributeSet_Damage.h"
#include "Components/OverdriveCombatComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "OverdriveCombatTags.h"

void UOverdriveCombatEffectExecution_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* AvatarActor = (TargetASC != nullptr) ? TargetASC->GetAvatarActor() : nullptr;
	if (AvatarActor == nullptr)
	{
		return;
	}

	UOverdriveCombatComponent* TargetCombat = AvatarActor->FindComponentByClass<UOverdriveCombatComponent>();
	if (TargetCombat == nullptr)
	{
		return;
	}

	TargetCombat->ExtendDamage(ExecutionParams, OutExecutionOutput);

	// 음수 데미지가 회복으로 뒤집히지 않도록 0 하한으로 클램핑한다. 0 이어도 모디파이어는 그대로 반영한다.
	const float PendingDamage = FMath::Max(0.0f, CalculatePendingDamage(ExecutionParams));

	// 여기서는 수치만 확정한다. 배리어/체력 분배와 이벤트 발송은 이 모디파이어가 실제로 반영된 뒤
	// UOverdriveCombatAttributeSet_Damage::PostGameplayEffectExecute 에서 이어진다.
	// 모디파이어를 하나로 두는 것이 그 훅을 정확히 1회만 부르게 하는 장치다.
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UOverdriveCombatAttributeSet_Damage::GetPendingDamageAttribute(), EGameplayModOp::AddBase, PendingDamage));
}

float UOverdriveCombatEffectExecution_Damage::CalculatePendingDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams) const
{
	const FGameplayEffectSpec& DamageEffectSpec = ExecutionParams.GetOwningSpec();

	const float BaseDamage = DamageEffectSpec.GetSetByCallerMagnitude(OverdriveCombatTags::Combat_SetByCaller_Damage, true, 0.0f);

	return BaseDamage;
}
