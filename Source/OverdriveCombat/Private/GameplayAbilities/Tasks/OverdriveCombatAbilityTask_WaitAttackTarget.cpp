// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/Tasks/OverdriveCombatAbilityTask_WaitAttackTarget.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "OverdriveCombatTags.h"

UOverdriveCombatAbilityTask_WaitAttackTarget::UOverdriveCombatAbilityTask_WaitAttackTarget()
{
	bOnlyTriggerOnce = false;
}

UOverdriveCombatAbilityTask_WaitAttackTarget* UOverdriveCombatAbilityTask_WaitAttackTarget::WaitAttackTarget(UGameplayAbility* OwningAbility, FGameplayTag EventTag, FName InInstanceName, bool bOnlyTriggerOnce)
{
	UOverdriveCombatAbilityTask_WaitAttackTarget* NewTask = NewAbilityTask<UOverdriveCombatAbilityTask_WaitAttackTarget>(OwningAbility, InInstanceName);

	NewTask->bOnlyTriggerOnce = bOnlyTriggerOnce;
	
	NewTask->EventTag = EventTag.IsValid() ? EventTag : OverdriveCombatTags::Combat_Event_Hit;

	return NewTask;
}

void UOverdriveCombatAbilityTask_WaitAttackTarget::Activate()
{
	if (!AbilitySystemComponent.IsValid())
	{
		EndTask();
		return;
	}

	UAbilitySystemComponent* AbilitySystem = AbilitySystemComponent.Get();

	EventHandle = AbilitySystem->GenericGameplayEventCallbacks.FindOrAdd(EventTag).AddUObject(this, &UOverdriveCombatAbilityTask_WaitAttackTarget::GameplayEventCallback);

	Super::Activate();
}

void UOverdriveCombatAbilityTask_WaitAttackTarget::OnDestroy(bool bInOwnerFinished)
{
	if (AbilitySystemComponent.IsValid() && EventHandle.IsValid())
	{
		AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(EventTag).Remove(EventHandle);
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UOverdriveCombatAbilityTask_WaitAttackTarget::GameplayEventCallback(const FGameplayEventData* Payload)
{
	if (Payload == nullptr)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		// 다중 히트의 진실은 TargetData 핸들에 있다 — 히트마다 하나씩 델리게이트로 흘려보낸다.
		for (const TSharedPtr<FGameplayAbilityTargetData>& TargetData : Payload->TargetData.Data)
		{
			const FGameplayAbilityTargetData* Data = TargetData.Get();
			if (Data == nullptr || Data->GetScriptStruct() != FOverdriveCombatTargetData_AttackHit::StaticStruct())
			{
				continue;
			}

			WaitAttackTargetDelegate.Broadcast(*static_cast<const FOverdriveCombatTargetData_AttackHit*>(Data));
		}
	}

	if (bOnlyTriggerOnce)
	{
		EndTask();
	}
}
