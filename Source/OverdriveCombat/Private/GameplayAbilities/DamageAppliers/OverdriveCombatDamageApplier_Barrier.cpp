// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/DamageAppliers/OverdriveCombatDamageApplier_Barrier.h"
#include "GameplayAbilities/OverdriveCombatBarrier.h"

void UOverdriveCombatDamageApplier_Barrier::ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec)
{
	UOverdriveCombatBarrier* Barrier = GetBarrier();
	check(Barrier);

	Barrier->ApplyDamage_Barrier(InDamage, Spec);
}

UOverdriveCombatBarrier* UOverdriveCombatDamageApplier_Barrier::GetBarrier() const
{
	return GetTypedOuter<UOverdriveCombatBarrier>();
}



