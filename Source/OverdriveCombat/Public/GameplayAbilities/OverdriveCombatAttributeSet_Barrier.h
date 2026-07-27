// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "OverdriveCombatAttributeSet_Barrier.generated.h"

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatAttributeSet_Barrier : public UAttributeSet
{
	GENERATED_BODY()
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_BarrierPoint)
	FGameplayAttributeData BarrierPoint;

	ATTRIBUTE_ACCESSORS_BASIC(UOverdriveCombatAttributeSet_Barrier, BarrierPoint);

	UFUNCTION()
	void OnRep_BarrierPoint(const FGameplayAttributeData& OldData);
};

