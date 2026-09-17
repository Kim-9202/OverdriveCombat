// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/OverdriveCombatBarrier.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilities/OverdriveCombatAttributeSet_Barrier.h"
#include "GameplayEffect.h"
#include "GameplayAbilities/DamageAppliers/OverdriveCombatDamageApplier_Barrier.h"
#include "Components/OverdriveCombatComponent.h"

UOverdriveCombatBarrier::UOverdriveCombatBarrier()
{
	RemainBarrier = 0.0f;
}

void UOverdriveCombatBarrier::OnBarrierDurationEnd(const FGameplayEffectRemovalInfo& InRemovalInfo)
{
	// 파괴 경로의 RemoveActiveGameplayEffect 가 이 콜백을 재진입시킨다.
	// 그 경우는 "지속시간 만료"가 아니므로 델리게이트를 쏘지 않는다.
	if (bRemoving)
	{
		return;
	}

	OnBarrierDurationEndDelegate.Broadcast(this, RemainBarrier);
	OnBarrierDurationEndDynamic.Broadcast(this, RemainBarrier);

	RemoveBarrier();
}

void UOverdriveCombatBarrier::OnBreakBarrier(float InFinalBreakDamage, const FGameplayEffectSpec& Spec)
{
	OnBreakBarrierDelegate.Broadcast(this, InFinalBreakDamage);
	OnBreakBarrierDynamic.Broadcast(this, InFinalBreakDamage);

	RemoveBarrier();
}

void UOverdriveCombatBarrier::ApplyDamage_Barrier(float& InDamage, const FGameplayEffectSpec& Spec)
{

	if (RemainBarrier <= 0.0f || !ensure(WeakASC.IsValid()))
	{
		RemoveBarrier();
		return;
	}

	UAbilitySystemComponent* TargetASC = WeakASC.Get();

	// 배율은 UOverdriveCombatEffectExecution_Damage 에서 이미 반영된 값이다.
	// 이 레이어는 남은 배리어만큼만 흡수하고 나머지는 다음 레이어로 넘긴다.
	const float AbsorbedDamage = FMath::Min(InDamage, RemainBarrier);
	InDamage -= AbsorbedDamage;

	// PostGameplayEffectExecute 안이므로 BaseValue 를 직접 수정한다(RemoveBarrier 와 동일 경로).
	TargetASC->ApplyModToAttribute(UOverdriveCombatAttributeSet_Barrier::GetBarrierPointAttribute(), EGameplayModOp::AddBase, -AbsorbedDamage);

	RemainBarrier -= AbsorbedDamage;

	if (RemainBarrier <= 0.0f)
	{
		OnBreakBarrier(AbsorbedDamage, Spec);
	}
}

void UOverdriveCombatBarrier::RemoveBarrier()
{
	// 파괴/만료/수동 제거가 겹쳐 들어와도 정리는 한 번만 수행한다.
	if (bRemoving)
	{
		return;
	}

	bRemoving = true;

	// A3: WeakASC 는 레벨 전환 등으로 먼저 죽을 수 있다. 역참조 전에 반드시 살아있는지 확인한다.
	if (UAbilitySystemComponent* ASC = WeakASC.Get())
	{
		if (0.0f < RemainBarrier)
		{
			ASC->ApplyModToAttribute(UOverdriveCombatAttributeSet_Barrier::GetBarrierPointAttribute(), EGameplayModOp::AddBase, -RemainBarrier);
		}

		if (BarrierGEHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(BarrierGEHandle);
		}
	}

	RemainBarrier = 0.0f;
	BarrierGEHandle.Invalidate();

	if (UOverdriveCombatComponent* Combat = WeakCombat.Get())
	{
		// Applier 는 약참조 소멸에 의존하지 말고 명시적으로 목록에서 내린다.
		Combat->RemoveOptionalDamageApplier(DamageApplier);

		// 마지막 강참조를 놓는다. 이후 외부 참조가 없으면 GC 가 자연 수거한다.
		Combat->UnregisterBarrier(this);
	}
}

bool UOverdriveCombatBarrier::ApplyBarrier(UOverdriveCombatComponent* TargetOCCombat, const FGameplayEffectSpec& InBarrierEffectSpec)
{
	check(!BarrierGEHandle.IsValid());

	if (TargetOCCombat == nullptr)
	{
		return false;
	}

	WeakCombat = TargetOCCombat;

	WeakASC = WeakCombat->GetLinkedAbilitySystem();
	if (!WeakASC.IsValid())
	{
		return false;
	}

	bool bFound = false;
	const float PrevBarrier = WeakASC->GetGameplayAttributeValue(UOverdriveCombatAttributeSet_Barrier::GetBarrierPointAttribute(), bFound);

	BarrierGEHandle = WeakASC->ApplyGameplayEffectSpecToSelf(InBarrierEffectSpec);

	const float CurrentBarrier = WeakASC->GetGameplayAttributeValue(UOverdriveCombatAttributeSet_Barrier::GetBarrierPointAttribute(), bFound);

	RemainBarrier = CurrentBarrier - PrevBarrier;
	if (RemainBarrier <= 0.0f)
	{
		// 등록 전이므로 참조가 없다 — 호출자가 nullptr 를 받으면 그대로 GC 수거된다.
		return false;
	}

	DamageApplier = NewObject<UOverdriveCombatDamageApplier_Barrier>(this);
	TargetOCCombat->AddOptionalDamageApplier(DamageApplier);

	FOnActiveGameplayEffectRemoved_Info* OnDurationGERemove = WeakASC->OnGameplayEffectRemoved_InfoDelegate(BarrierGEHandle);

	if (OnDurationGERemove)
	{
		OnDurationGERemove->AddUObject(this, &UOverdriveCombatBarrier::OnBarrierDurationEnd);
	}

	// A1: 대상 컴포넌트가 강참조로 수명을 보증한다(약참조/Outer 는 GC 를 못 막는다).
	TargetOCCombat->RegisterBarrier(this);

	return true;
}


