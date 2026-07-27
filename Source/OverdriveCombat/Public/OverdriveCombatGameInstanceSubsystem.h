// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OverdriveCombatGameInstanceSubsystem.generated.h"

class UOverdriveCombatDamageApplier;

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//virtual void Deinitialize() override;
};

