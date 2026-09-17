// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder_Owner.h"
#include "AbilitySystemGlobals.h"
#include "Components/ActorComponent.h"

UAbilitySystemComponent* UOverdriveCombatAbilitySystemFinder_Owner::FindAbilitySystem_Implementation(UActorComponent* InComponent) const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InComponent->GetOwner());
}
