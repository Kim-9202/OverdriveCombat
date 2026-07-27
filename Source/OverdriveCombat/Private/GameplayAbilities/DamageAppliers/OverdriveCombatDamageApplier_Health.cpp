// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/DamageAppliers/OverdriveCombatDamageApplier_Health.h"
#include "GameplayAbilities/OverdriveCombatAttributeSet_Health.h"

void UOverdriveCombatDamageApplier_Health::ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec)
{
	if (InDamage <= 0.0f)
	{
		return;
	}

	// 배율은 UOverdriveCombatEffectExecution_Damage 에서 이미 반영된 값이다.
	// 이 레이어는 종단 소비자이므로 남은 데미지를 전부 소진한다.
	const float FinalDamage = InDamage;
	InDamage = 0.0f;

	AddAttributeModifier(UOverdriveCombatAttributeSet_Health::GetHealthPointAttribute(), EGameplayModOp::AddBase, -FinalDamage);
}
