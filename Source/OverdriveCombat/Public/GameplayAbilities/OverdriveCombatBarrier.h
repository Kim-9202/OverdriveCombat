// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffectTypes.h"
#include "OverdriveCombatBarrier.generated.h"

class UOverdriveCombatBarrier;
class UOverdriveCombatDamageApplier_Barrier;
class UAbilitySystemComponent;
struct FGameplayEffectSpec;
class UOverdriveCombatComponent;

/** 배리어가 데미지로 파괴될 때(C++). 파괴를 일으킨 데미지 실행 컨텍스트를 그대로 받는다. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOverdriveCombatOnBreakBarrierDelegate, UOverdriveCombatBarrier*, float);

/** 배리어 GE 지속시간이 다 되어 제거될 때(C++). 파라미터는 만료 시점의 잔여 배리어. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOverdriveCombatOnBarrierDurationEndDelegate, UOverdriveCombatBarrier*, float);

/** 배리어가 데미지로 파괴될 때(BP). 실행 컨텍스트까지 필요하면 C++ 델리게이트를 쓴다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOverdriveCombatOnBreakBarrierDynamic, UOverdriveCombatBarrier*, Barrier, float, FinalBreakDamage);

/** 배리어 GE 지속시간이 다 되어 제거될 때(BP). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOverdriveCombatOnBarrierDurationEndDynamic, UOverdriveCombatBarrier*, Barrier, float, RemainBarrier);

/**
 * 대상의 데미지 파이프라인 앞단에 끼어드는 배리어(보호막).
 *
 * 수명은 대상 UOverdriveCombatComponent 가 강참조(RegisterBarrier)로 보증한다 —
 * Outer 체인이나 약참조는 GC 를 막지 못하므로, 등록 전/해제 후의 배리어는 수거 대상이다.
 * 제거(RemoveBarrier)는 GAS 상태 정리 후 등록을 해제해 자연 수거되게 한다(MarkAsGarbage 금지 —
 * 외부가 들고 있는 참조를 밑에서 죽이지 않는다).
 */
UCLASS(BlueprintType, Blueprintable)
class OVERDRIVECOMBAT_API UOverdriveCombatBarrier : public UObject
{
	GENERATED_BODY()
public:
	UOverdriveCombatBarrier();

private:
	UPROPERTY()
	TObjectPtr<UOverdriveCombatDamageApplier_Barrier> DamageApplier;

	TWeakObjectPtr<UAbilitySystemComponent> WeakASC;

	/** 이 배리어를 소유(강참조)한 대상의 컴포넌트. 제거 시 등록 해제 통로. */
	TWeakObjectPtr<UOverdriveCombatComponent> WeakCombat;

	FActiveGameplayEffectHandle BarrierGEHandle;

	FOverdriveCombatOnBreakBarrierDelegate OnBreakBarrierDelegate;

	FOverdriveCombatOnBarrierDurationEndDelegate OnBarrierDurationEndDelegate;

	float RemainBarrier;

	/** 제거가 시작됐는지. GE 제거 콜백 재진입(파괴 경로의 RemoveActiveGameplayEffect)과 중복 제거를 차단한다. */
	bool bRemoving = false;

protected:
	virtual void OnBarrierDurationEnd(const FGameplayEffectRemovalInfo& InRemovalInfo);

	virtual void OnBreakBarrier(float InFinalBreakDamage, const FGameplayEffectSpec& Spec);

public:
	/** 배리어 파괴 시(BP). 데미지 실행 도중(서버) 호출된다. */
	UPROPERTY(BlueprintAssignable, Category = "OverdriveCombat")
	FOverdriveCombatOnBreakBarrierDynamic OnBreakBarrierDynamic;

	/** 지속시간 만료 시(BP). 파괴로 제거될 때는 호출되지 않는다. */
	UPROPERTY(BlueprintAssignable, Category = "OverdriveCombat")
	FOverdriveCombatOnBarrierDurationEndDynamic OnBarrierDurationEndDynamic;

	/** 배리어 파괴 시(C++). 멀티캐스트이므로 Add/Remove 로 붙인다. */
	FOverdriveCombatOnBreakBarrierDelegate& GetOnBreakBarrierDelegate() { return OnBreakBarrierDelegate; }

	/** 지속시간 만료 시(C++). 멀티캐스트이므로 Add/Remove 로 붙인다. */
	FOverdriveCombatOnBarrierDurationEndDelegate& GetOnBarrierDurationEndDelegate() { return OnBarrierDurationEndDelegate; }

	UFUNCTION(BlueprintPure, Category = "OverdriveCombat")
	float GetRemainBarrier() const { return RemainBarrier; }

	/** 데미지 파이프라인의 배리어 레이어. Spec 으로 공격 유형을 판별해 흡수 규칙을 달리할 수 있다. */
	virtual void ApplyDamage_Barrier(float& InDamage, const FGameplayEffectSpec& Spec);

	void RemoveBarrier();

	bool ApplyBarrier(UOverdriveCombatComponent* TargetOCCombat, const FGameplayEffectSpec& InBarrierEffectSpec);
};

