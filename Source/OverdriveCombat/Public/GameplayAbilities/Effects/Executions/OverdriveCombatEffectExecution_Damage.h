// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "OverdriveCombatEffectExecution_Damage.generated.h"

/**
 * 데미지 수치를 확정하는 실행체.
 *
 * 어트리뷰트 캡처와 배율 계산은 전부 여기서 끝난다 — DamageApplier 파이프라인은 확정된 수치를
 * 배리어·체력에 나눠 소비하기만 한다(캡처를 훅 안에서 즉석으로 하지 않는다).
 */
UCLASS()
class OVERDRIVECOMBAT_API UOverdriveCombatEffectExecution_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	/**
	 * SetByCaller 기본 데미지 취득 · 배율 어트리뷰트 캡처 · 합성까지 끝내 최종 PendingDamage 를 산출한다.
	 * 데미지 수치 계산 로직을 바꾸거나 파생 실행체에서 재정의할 단일 지점. 유효 데미지가 없으면 0 을 반환한다.
	 */
	virtual float CalculatePendingDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams) const;

};

