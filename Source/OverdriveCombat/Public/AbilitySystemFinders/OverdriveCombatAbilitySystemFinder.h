// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/TimerHandle.h"
#include "OverdriveCombatAbilitySystemFinder.generated.h"

class UAbilitySystemComponent;
class UActorComponent;

DECLARE_DELEGATE_OneParam(FOnOverdriveCombatAbilitySystemFoundSignature, UAbilitySystemComponent*);
DECLARE_DELEGATE(FOnOverdriveCombatAbilitySystemFindFailedSignature);

/**
 * 컴포넌트가 쓸 AbilitySystem 을 주기적으로 재시도하며 찾는다.
 *
 * ASC 가 PlayerState 처럼 폰보다 늦게 확정되는 액터에 있는 구성에서는 BeginPlay 시점의 1회 조회가 반드시 실패한다
 * (스폰 → BeginPlay → Possess 순서라 그 시점 폰의 PlayerState 는 아직 null).
 * 탐색 전략을 클래스로 분리해 두면 같은 컴포넌트를 오너 보유형·PlayerState 보유형 양쪽에 그대로 쓸 수 있다.
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class OVERDRIVECOMBAT_API UOverdriveCombatAbilitySystemFinder : public UObject
{
	GENERATED_BODY()

public:
	FOnOverdriveCombatAbilitySystemFoundSignature OnFound;
	FOnOverdriveCombatAbilitySystemFindFailedSignature OnFailed;

	TWeakObjectPtr<UActorComponent> WeakOwnerComponent;

	float RetryPeriod = 0.1f;

	int32 MaxAttemptCount = 1;

	void StartFind();

	void StopFind();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "OverdriveCombat")
	UAbilitySystemComponent* FindAbilitySystem(UActorComponent* InComponent) const;
	virtual UAbilitySystemComponent* FindAbilitySystem_Implementation(UActorComponent* InComponent) const;

private:
	int32 CurrentAttemptCount = 0;

	FTimerHandle RetryTimerHandle;

	void TryFind();

	void FinishFind(UAbilitySystemComponent* FoundAbilitySystem);
};
