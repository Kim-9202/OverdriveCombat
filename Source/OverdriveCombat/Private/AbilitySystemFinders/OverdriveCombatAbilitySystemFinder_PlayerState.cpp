// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder_PlayerState.h"
#include "AbilitySystemGlobals.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UAbilitySystemComponent* UOverdriveCombatAbilitySystemFinder_PlayerState::FindAbilitySystem_Implementation(UActorComponent* InComponent) const
{
	AActor* OwnerActor = InComponent->GetOwner();

	APlayerState* TargetPlayerState = Cast<APlayerState>(OwnerActor);

	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		TargetPlayerState = OwnerPawn->GetPlayerState();
	}
	else if (const AController* OwnerController = Cast<AController>(OwnerActor))
	{
		TargetPlayerState = OwnerController->PlayerState;
	}

	if (!IsValid(TargetPlayerState))
	{
		return nullptr;
	}

	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetPlayerState);
}
