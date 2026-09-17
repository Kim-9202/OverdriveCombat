// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder.h"
#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UOverdriveCombatAbilitySystemFinder::StartFind()
{
	if (RetryTimerHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		FinishFind(nullptr);
		return;
	}

	World->GetTimerManager().SetTimer(RetryTimerHandle, this, &UOverdriveCombatAbilitySystemFinder::TryFind, RetryPeriod, true);

	TryFind();
}

void UOverdriveCombatAbilitySystemFinder::StopFind()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}

	RetryTimerHandle.Invalidate();
	CurrentAttemptCount = 0;
}

UAbilitySystemComponent* UOverdriveCombatAbilitySystemFinder::FindAbilitySystem_Implementation(UActorComponent* InComponent) const
{
	return nullptr;
}

void UOverdriveCombatAbilitySystemFinder::TryFind()
{
	UActorComponent* OwnerComponent = WeakOwnerComponent.Get();

	if (!IsValid(OwnerComponent))
	{
		FinishFind(nullptr);
		return;
	}

	++CurrentAttemptCount;

	if (UAbilitySystemComponent* FoundAbilitySystem = FindAbilitySystem(OwnerComponent))
	{
		FinishFind(FoundAbilitySystem);
		return;
	}

	if (CurrentAttemptCount >= MaxAttemptCount)
	{
		FinishFind(nullptr);
	}
}

void UOverdriveCombatAbilitySystemFinder::FinishFind(UAbilitySystemComponent* FoundAbilitySystem)
{
	StopFind();

	if (IsValid(FoundAbilitySystem))
	{
		OnFound.ExecuteIfBound(FoundAbilitySystem);
	}
	else
	{
		OnFailed.ExecuteIfBound();
	}
}
