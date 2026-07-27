// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OverdriveCombatDamageApplier.h"
#include "OverdriveCombatDamageApplier_Health.generated.h"

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatDamageApplier_Health : public UOverdriveCombatDamageApplier
{
	GENERATED_BODY()
	

protected:
	virtual void ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec) override;

};


