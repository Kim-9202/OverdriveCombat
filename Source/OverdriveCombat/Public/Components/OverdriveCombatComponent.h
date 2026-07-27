// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "OverdriveCombatComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UOverdriveCombatBarrier;
class UOverdriveCombatDamageApplier;
class UOverdriveCombatDamageExtender;
struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectCustomExecutionOutput;
struct FGameplayEffectSpec;

UCLASS( ClassGroup=(OverdriveCombat), meta=(BlueprintSpawnableComponent) )
class OVERDRIVECOMBAT_API UOverdriveCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOverdriveCombatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
protected:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	TWeakObjectPtr<UAbilitySystemComponent> WeakASC;

	void LinkAbilitySystem();
	void UnlinkAbilitySystem();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> HitStopEffect;

	FActiveGameplayEffectHandle HitStopEffectHandle;

public:
	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void ApplyHitStop(float NewTime);

	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void ClearHitStop();

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<UOverdriveCombatDamageApplier>> OptionalDamageAppliers;

	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", Instanced, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UOverdriveCombatDamageApplier>> StartOptionalDamageAppliers;

	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", Instanced, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOverdriveCombatDamageApplier> DefaultDamageApplier;

	void CheckDamageApplier();

public:
	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void AddOptionalDamageApplier(UOverdriveCombatDamageApplier* NewApplier);

	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void RemoveOptionalDamageApplier(UOverdriveCombatDamageApplier* InApplier);

	/**
	 * PendingDamage 소비 지점(UOverdriveCombatAttributeSet_Damage::PostGameplayEffectExecute)에서 호출된다.
	 * DamageApplier 파이프라인으로 배리어·체력에 분배한다. 이벤트 발송은 호출자가 담당한다.
	 */
	void ApplyDamage(float InDamage, const FGameplayEffectSpec& Spec);

private:
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UOverdriveCombatDamageExtender>> DamageExtenders;

public:
	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void AddDamageExtender(TSubclassOf<UOverdriveCombatDamageExtender> NewExtender);

	UFUNCTION(BlueprintCallable, Category = "OverdriveCombat")
	void RemoveDamageExtender(TSubclassOf<UOverdriveCombatDamageExtender> InExtender);

	void ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const;

	UAbilitySystemComponent* GetLinkedAbilitySystem() const { return WeakASC.Get(); }

private:
	/**
	 * 활성 배리어 강참조. 배리어 수명은 이 배열이 보증한다 —
	 * Outer 체인이나 약참조(OptionalDamageAppliers)는 GC 를 막지 못하므로,
	 * 여기 등록되지 않은 배리어는 호출자가 참조를 놓는 순간 수거된다.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UOverdriveCombatBarrier>> ActiveBarriers;

public:
	/** 배리어 적용 성공 시(ApplyBarrier) 배리어가 자신을 등록해 GC 로부터 보호한다. */
	void RegisterBarrier(UOverdriveCombatBarrier* InBarrier);

	/** 배리어 제거 시(RemoveBarrier) 등록 해제. 이후 남은 참조가 없으면 GC 가 수거한다. */
	void UnregisterBarrier(UOverdriveCombatBarrier* InBarrier);
};

