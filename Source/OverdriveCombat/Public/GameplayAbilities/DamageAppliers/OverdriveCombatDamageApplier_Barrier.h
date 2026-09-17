// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OverdriveCombatDamageApplier.h"
#include "OverdriveCombatDamageApplier_Barrier.generated.h"

class UOverdriveCombatBarrier;

/**
 * Outer 가 반드시 UOverdriveCombatBarrier 여야 하므로(ApplyBarrier 의 NewObject 가 유일한 생성처),
 * 컴포넌트의 Instanced 슬롯 피커와 BP 부모 목록에서 감춘다. 노출하면 디자이너가 고르는 순간 Outer 가
 * 컴포넌트가 되어 GetBarrier() 가 null 이 된다.
 */
UCLASS(NotEditInlineNew, HideDropDown, NotBlueprintable)
class OVERDRIVECOMBAT_API UOverdriveCombatDamageApplier_Barrier : public UOverdriveCombatDamageApplier
{
	GENERATED_BODY()

protected:
	virtual void ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec) override;

public:
	UOverdriveCombatBarrier* GetBarrier() const;
};


