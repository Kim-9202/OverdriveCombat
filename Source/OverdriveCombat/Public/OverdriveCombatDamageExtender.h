// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "OverdriveCombatDamageExtender.generated.h"

struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectCustomExecutionOutput;

/**
 * 데미지 수치가 확정되기 전에 끼어드는 무상태 확장점.
 * 인스턴스를 만들지 않고 CDO 에서 직접 실행되므로(UOverdriveCombatComponent::ExtendDamage) 상태를 멤버에 쌓아 두면 안 된다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class OVERDRIVECOMBAT_API UOverdriveCombatDamageExtender : public UObject
{
	GENERATED_BODY()

private:
	// CDO 에서 호출되므로 인스턴스 멤버가 아니다. 호출당 스크래치이며 Extend 가 반환하면 이전 값으로 되돌린다.
	mutable const FGameplayEffectCustomExecutionParameters* CachedExecutionParams = nullptr;
	mutable FGameplayEffectCustomExecutionOutput* CachedExecutionOutput = nullptr;

public:
	void ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "OverdriveCombat")
	void Extend(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;
	virtual void Extend_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const { }

	// Extend 실행 중에만 유효하다.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void MarkGameplayCuesHandledManually() const;

	// Extend 실행 중에만 유효하다.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void AddDynamicAssetTag(FGameplayTag TagToAdd) const;

	// Extend 실행 중에만 유효하다.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void AppendDynamicAssetTags(FGameplayTagContainer TagsToAppend) const;
};
