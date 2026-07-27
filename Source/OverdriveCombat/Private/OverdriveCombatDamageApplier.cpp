// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatDamageApplier.h"
#include "Components/OverdriveCombatComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UOverdriveCombatDamageApplier::AddAttributeModifier(const FGameplayAttribute& Attribute, TEnumAsByte<EGameplayModOp::Type> ModOp, float Magnitude) const
{
	if (!Attribute.IsValid())
	{
		return;
	}

	UOverdriveCombatComponent* Combat = GetLinkedCombatComponent();
	if (!ensureMsgf(Combat != nullptr, TEXT("[OverdriveCombat] %s: CombatComponent 에 연결되지 않아 %s 변화가 유실됩니다."), *GetNameSafe(this), *Attribute.GetName()))
	{
		return;
	}

	UAbilitySystemComponent* ASC = Combat->GetLinkedAbilitySystem();
	if (!ensureMsgf(ASC != nullptr, TEXT("[OverdriveCombat] %s: 대상 ASC 가 없어 %s 변화가 유실됩니다."), *GetNameSafe(this), *Attribute.GetName()))
	{
		return;
	}

	// PostGameplayEffectExecute(서버 전용) 안에서 호출된다. ApplyModToAttribute 는 InternalExecuteMod 를
	// 거치지 않으므로 PostGameplayEffectExecute 를 재귀 호출하지 않는다.
	ASC->ApplyModToAttribute(Attribute, ModOp, Magnitude);
}

void UOverdriveCombatDamageApplier::LinkCombatComponent(UOverdriveCombatComponent* InCombat)
{
	if (InCombat)
	{
		LinkedCombat = InCombat;
	}
}

void UOverdriveCombatDamageApplier::UnlinkCombatComponent()
{
	if (LinkedCombat.IsValid())
	{
		LinkedCombat = nullptr;
	}
}


