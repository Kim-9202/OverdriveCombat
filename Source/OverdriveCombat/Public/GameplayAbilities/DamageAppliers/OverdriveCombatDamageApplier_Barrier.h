// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OverdriveCombatDamageApplier.h"
#include "OverdriveCombatDamageApplier_Barrier.generated.h"

class UOverdriveCombatBarrier;

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatDamageApplier_Barrier : public UOverdriveCombatDamageApplier
{
	GENERATED_BODY()
	
private:
	UOverdriveCombatDamageApplier_Barrier();

private:


protected:
	virtual void ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec) override;

public:
	UOverdriveCombatBarrier* GetBarrier() const;
};


