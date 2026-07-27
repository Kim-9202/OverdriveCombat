// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatDamageExtender.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffect.h"

void UOverdriveCombatDamageExtender::ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Restore the previous cache afterwards so a reentrant Extend (e.g. applying a GE inside Extend) keeps the outer call valid.
	const FGameplayEffectCustomExecutionParameters* PrevExecutionParams = CachedExecutionParams;
	FGameplayEffectCustomExecutionOutput* PrevExecutionOutput = CachedExecutionOutput;

	CachedExecutionParams = &ExecutionParams;
	CachedExecutionOutput = &OutExecutionOutput;

	Extend(ExecutionParams, OutExecutionOutput);

	CachedExecutionParams = PrevExecutionParams;
	CachedExecutionOutput = PrevExecutionOutput;
}

void UOverdriveCombatDamageExtender::MarkGameplayCuesHandledManually() const
{
	if (CachedExecutionOutput == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOverdriveCombatDamageExtender::MarkGameplayCuesHandledManually is only valid while Extend is running."));
		return;
	}

	CachedExecutionOutput->MarkGameplayCuesHandledManually();
}

void UOverdriveCombatDamageExtender::AddDynamicAssetTag(FGameplayTag TagToAdd) const
{
	FGameplayEffectSpec* OwningSpec = CachedExecutionParams ? CachedExecutionParams->GetOwningSpecForPreExecuteMod() : nullptr;
	if (OwningSpec == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOverdriveCombatDamageExtender::AddDynamicAssetTag is only valid while Extend is running."));
		return;
	}

	OwningSpec->AddDynamicAssetTag(TagToAdd);
}

void UOverdriveCombatDamageExtender::AppendDynamicAssetTags(FGameplayTagContainer TagsToAppend) const
{
	FGameplayEffectSpec* OwningSpec = CachedExecutionParams ? CachedExecutionParams->GetOwningSpecForPreExecuteMod() : nullptr;
	if (OwningSpec == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOverdriveCombatDamageExtender::AppendDynamicAssetTags is only valid while Extend is running."));
		return;
	}

	OwningSpec->AppendDynamicAssetTags(TagsToAppend);
}
