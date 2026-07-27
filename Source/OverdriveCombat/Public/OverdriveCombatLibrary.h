// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"
#include "OverdriveCombatLibrary.generated.h"

class UOverdriveCombatComponent;
class UOverdriveCombatBarrier;
class UGameplayEffect;
struct FGameplayEffectSpecHandle;

/**
 * 
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat", meta =(DeterminesOutputType = "NewBarrierClass", DynamicOutputParam = "ReturnValue"))
	static UOverdriveCombatBarrier* CreateBarrier(UOverdriveCombatComponent* TargetOCCombat, TSubclassOf<UOverdriveCombatBarrier> NewBarrierClass, const FGameplayEffectSpecHandle& InBarrierEffectSpec);

	/**
	 * TargetData 핸들의 Index 번째 항목이 FOverdriveCombatTargetData_AttackHit 이면 AttackTypeTag 를 돌려준다.
	 * 아니면 빈 태그를 돌려준다. Combat.Event.Hit 수신 어빌리티에서 공격 타입을 구분할 때 쓴다.
	 */
	UFUNCTION(BlueprintPure, Category = "OverdriveCombat|TargetData")
	static FGameplayTag GetAttackTypeTagFromTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, int32 Index = 0);

	/** FOverdriveCombatTargetData_AttackHit 를 구성 요소로 분해한다(BP Break 노드). */
	UFUNCTION(BlueprintPure, Category = "OverdriveCombat|TargetData", meta = (NativeBreakFunc))
	static void BreakAttackHitTargetData(const FOverdriveCombatTargetData_AttackHit& AttackHit, FHitResult& HitResult, FGameplayTag& AttackTypeTag, FVector& Origin);

	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	static FGameplayEffectContextHandle DuplicateEffectContextHandle(const FGameplayEffectContextHandle& InHandle);
};

