// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"
#include "OverdriveCombatAbilityTask_WaitAttackTarget.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitAttackTargetDelegate, const FOverdriveCombatTargetData_AttackHit&, TargetData);

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatAbilityTask_WaitAttackTarget : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UOverdriveCombatAbilityTask_WaitAttackTarget();

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true", DisplayName = "Wait Attack Target (AbilityTask)"))
	static UOverdriveCombatAbilityTask_WaitAttackTarget* WaitAttackTarget(UGameplayAbility* OwningAbility, UPARAM(meta = (Categories = "Combat.Event.Hit"))FGameplayTag EventTag, FName InInstanceName, bool bOnlyTriggerOnce = false);

	UPROPERTY(BlueprintAssignable)
	FWaitAttackTargetDelegate WaitAttackTargetDelegate;

protected:
	virtual void Activate() override;

	virtual void OnDestroy(bool bInOwnerFinished) override;

	virtual void GameplayEventCallback(const FGameplayEventData* Payload);

private:
	FGameplayTag EventTag;

	bool bOnlyTriggerOnce;

	FDelegateHandle EventHandle;

};
