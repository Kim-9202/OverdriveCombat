// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatDamageExtender.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffect.h"

void UOverdriveCombatDamageExtender::ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// 끝나고 이전 캐시로 되돌린다. Extend 안에서 또 GE 를 적용하는 식으로 재진입해도 바깥 호출이 계속 유효하도록.
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
