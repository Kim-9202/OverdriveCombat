// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "OverdriveCombatAttributeSet_Health.generated.h"

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatAttributeSet_Health : public UAttributeSet
{
	GENERATED_BODY()
	

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_MaxHealthPoint)
	FGameplayAttributeData MaxHealthPoint;
	ATTRIBUTE_ACCESSORS_BASIC(UOverdriveCombatAttributeSet_Health, MaxHealthPoint);

	UFUNCTION()
	void OnRep_MaxHealthPoint(const FGameplayAttributeData& OldData);

	UPROPERTY(ReplicatedUsing = OnRep_HealthPoint)
	FGameplayAttributeData HealthPoint;
	ATTRIBUTE_ACCESSORS_BASIC(UOverdriveCombatAttributeSet_Health, HealthPoint);

	UFUNCTION()
	void OnRep_HealthPoint(const FGameplayAttributeData& OldData);
};

