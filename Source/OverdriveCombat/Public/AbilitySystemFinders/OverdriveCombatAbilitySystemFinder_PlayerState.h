// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder.h"
#include "OverdriveCombatAbilitySystemFinder_PlayerState.generated.h"

/**
 *
 */
UCLASS(BlueprintType, DisplayName = "Find From PlayerState")
class OVERDRIVECOMBAT_API UOverdriveCombatAbilitySystemFinder_PlayerState : public UOverdriveCombatAbilitySystemFinder
{
	GENERATED_BODY()

protected:
	virtual UAbilitySystemComponent* FindAbilitySystem_Implementation(UActorComponent* InComponent) const override;
};
