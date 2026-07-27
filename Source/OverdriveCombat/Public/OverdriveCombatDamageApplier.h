// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayEffect.h"
#include "OverdriveCombatDamageApplier.generated.h"

class UAbilitySystemComponent;
class UOverdriveCombatComponent;

/**
 *
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class OVERDRIVECOMBAT_API UOverdriveCombatDamageApplier : public UObject
{
	GENERATED_BODY()

private:
	TWeakObjectPtr<UOverdriveCombatComponent> LinkedCombat;

public:
	/**
	 * 데미지 파이프라인 한 단계. InDamage 를 소비/변형하고 AddAttributeModifier 로 어트리뷰트에 반영한다.
	 *
	 * AttributeSet 의 PostGameplayEffectExecute 안에서 호출된다(서버 전용). Spec 으로 GE 의 에셋 태그·
	 * SetByCaller·캡처 태그·EffectContext 를 모두 볼 수 있어, 공격 유형에 따라 다르게 반응할 수 있다.
	 * Spec 은 호출 구간에서만 유효한 참조다 — 멤버로 보관하지 말 것.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "OverdriveCombat")
	void ApplyDamage(UPARAM(ref) float& InDamage, const FGameplayEffectSpec& Spec);
	virtual void ApplyDamage_Implementation(float& InDamage, const FGameplayEffectSpec& Spec) { }

	/** 연결된 대상 ASC 에 (Attribute, ModOp, Magnitude) 변화를 즉시 반영하는 헬퍼. BP override 에서 결과를 적용할 때 사용. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "OverdriveCombat")
	void AddAttributeModifier(const FGameplayAttribute& Attribute, TEnumAsByte<EGameplayModOp::Type> ModOp, float Magnitude) const;

	virtual void LinkCombatComponent(UOverdriveCombatComponent* InCombat);

	virtual void UnlinkCombatComponent();

	UOverdriveCombatComponent* GetLinkedCombatComponent() const { return LinkedCombat.Get(); }
};

