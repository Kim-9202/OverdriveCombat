// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "OverdriveCombatDamageExtender.generated.h"

struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectCustomExecutionOutput;

/**
 * Stateless extender invoked on its CDO before the damage execution runs.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class OVERDRIVECOMBAT_API UOverdriveCombatDamageExtender : public UObject
{
	GENERATED_BODY()

private:
	// Invoked on the CDO, so these are per-call scratch state restored after Extend returns.
	mutable const FGameplayEffectCustomExecutionParameters* CachedExecutionParams = nullptr;
	mutable FGameplayEffectCustomExecutionOutput* CachedExecutionOutput = nullptr;

public:
	void ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "OverdriveCombat")
	void Extend(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;
	virtual void Extend_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const { }

	// Only valid while Extend is running.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void MarkGameplayCuesHandledManually() const;

	// Only valid while Extend is running.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void AddDynamicAssetTag(FGameplayTag TagToAdd) const;

	// Only valid while Extend is running.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void AppendDynamicAssetTags(FGameplayTagContainer TagsToAppend) const;
};
