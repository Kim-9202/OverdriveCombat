// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "OverdriveCombatAttributeSet_Damage.generated.h"

/**
 * 데미지 파이프라인의 단일 진입점.
 *
 * PendingDamage 는 메타 어트리뷰트다 — Execution 이 계산한 데미지를 담는 임시 통로일 뿐이며,
 * PostGameplayEffectExecute 에서 즉시 소비되어 0 으로 돌아간다. 복제하지 않는다.
 *
 * 데미지 모디파이어를 이 하나로 모으는 이유: PostGameplayEffectExecute 는 모디파이어마다 호출되므로,
 * 체력·배리어에 각각 emit 하면 둘 다 깎일 때 훅이 2회 불리고(중복) 배리어만 흡수할 때는
 * 체력 훅이 아예 안 불린다(누락). 진입점을 하나로 두면 GE 적용 1회 = 훅 1회가 구조적으로 보장된다.
 * 실제 체력·배리어 분배는 UOverdriveCombatComponent 의 DamageApplier 파이프라인이 담당한다.
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatAttributeSet_Damage : public UAttributeSet
{
	GENERATED_BODY()

protected:
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/**
	 * 데미지가 배리어·체력에 실제로 분배된 뒤 호출되는 후처리 훅.
	 * 기본 구현은 타겟에 Combat.Event.Damage 를, 가해자가 있으면 Combat.Event.OnHit 을 보낸다.
	 */
	virtual void PostDamageAction(float InDamage, const struct FGameplayEffectModCallbackData& Data);

public:
	/** 메타 어트리뷰트. 복제하지 않으며 소비 즉시 0 으로 초기화된다. */
	UPROPERTY()
	FGameplayAttributeData PendingDamage;

	ATTRIBUTE_ACCESSORS_BASIC(UOverdriveCombatAttributeSet_Damage, PendingDamage);
};
