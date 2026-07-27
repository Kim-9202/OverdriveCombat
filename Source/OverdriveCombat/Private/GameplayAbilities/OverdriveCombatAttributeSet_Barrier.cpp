// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/OverdriveCombatAttributeSet_Barrier.h"
#include "Net/UnrealNetwork.h"

void UOverdriveCombatAttributeSet_Barrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UOverdriveCombatAttributeSet_Barrier, BarrierPoint, COND_None, REPNOTIFY_OnChanged);
}

void UOverdriveCombatAttributeSet_Barrier::OnRep_BarrierPoint(const FGameplayAttributeData& OldData)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverdriveCombatAttributeSet_Barrier, BarrierPoint, OldData)
}


