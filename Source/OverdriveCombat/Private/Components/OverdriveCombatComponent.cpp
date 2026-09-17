// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/OverdriveCombatComponent.h"
#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder.h"
#include "AbilitySystemFinders/OverdriveCombatAbilitySystemFinder_Owner.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"
#include "OverdriveCombatDamageApplier.h"
#include "OverdriveCombatDamageExtender.h"
#include "GameplayAbilities/OverdriveCombatBarrier.h"
#include "GameplayEffectExecutionCalculation.h"

// Sets default values for this component's properties
UOverdriveCombatComponent::UOverdriveCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	AbilitySystemFinderClass = UOverdriveCombatAbilitySystemFinder_Owner::StaticClass();
}


// Called when the game starts
void UOverdriveCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	InitializeAbilitySystem();

	// Applier 는 AddAttributeModifier 에서 이 컴포넌트를 통해 대상 ASC 를 찾는다.
	// 기본 Applier 도 반드시 연결해야 데미지가 유실되지 않는다.
	if (DefaultDamageApplier != nullptr)
	{
		DefaultDamageApplier->LinkCombatComponent(this);
	}

	for (auto& StartOptionalDamageApplier : StartOptionalDamageAppliers)
	{
		AddOptionalDamageApplier(StartOptionalDamageApplier);
	}
}

void UOverdriveCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAbilitySystemFinder();

	// 아래 정리는 전부 대상 ASC 를 거치므로 링크를 끊기 전에 끝낸다.
	// ASC 가 PlayerState 처럼 이 컴포넌트보다 오래 사는 액터에 있으면, 안 걷어낸 GE 가 그대로 남는다.
	ClearHitStop();

	// RemoveBarrier 가 UnregisterBarrier 로 ActiveBarriers 를 건드리므로 스냅샷을 순회한다.
	const TArray<TObjectPtr<UOverdriveCombatBarrier>> BarriersSnapshot = ActiveBarriers;
	for (UOverdriveCombatBarrier* Barrier : BarriersSnapshot)
	{
		if (IsValid(Barrier))
		{
			Barrier->RemoveBarrier();
		}
	}
	ActiveBarriers.Reset();

	// 배리어가 들고 있던 Applier 는 위 RemoveBarrier 에서 이미 내려갔다. 남은 것은 StartOptional 계열이다.
	for (const TWeakObjectPtr<UOverdriveCombatDamageApplier>& OptionalApplier : OptionalDamageAppliers)
	{
		if (UOverdriveCombatDamageApplier* Applier = OptionalApplier.Get())
		{
			Applier->UnlinkCombatComponent();
		}
	}
	OptionalDamageAppliers.Reset();

	if (DefaultDamageApplier != nullptr)
	{
		DefaultDamageApplier->UnlinkCombatComponent();
	}

	UnlinkAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult UOverdriveCombatComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (DefaultDamageApplier == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("Default Damage Applier Is Invalid")));
		Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
	}

	return Result;
}
#endif

void UOverdriveCombatComponent::SetAbilitySystemFinderClass(TSubclassOf<UOverdriveCombatAbilitySystemFinder> InFinderClass)
{
	if (!ensure(InFinderClass))
	{
		return;
	}

	AbilitySystemFinderClass = InFinderClass;
}

void UOverdriveCombatComponent::InitializeAbilitySystem()
{
	if (!ensure(AbilitySystemFinderClass))
	{
		return;
	}

	AbilitySystemFinder = NewObject<UOverdriveCombatAbilitySystemFinder>(this, AbilitySystemFinderClass);
	AbilitySystemFinder->WeakOwnerComponent = this;
	AbilitySystemFinder->RetryPeriod = AbilitySystemFindPeriod;
	AbilitySystemFinder->MaxAttemptCount = AbilitySystemFindMaxCount;
	AbilitySystemFinder->OnFound.BindUObject(this, &UOverdriveCombatComponent::HandleAbilitySystemFound);
	AbilitySystemFinder->OnFailed.BindUObject(this, &UOverdriveCombatComponent::HandleAbilitySystemFindFailed);

	AbilitySystemFinder->StartFind();
}

void UOverdriveCombatComponent::HandleAbilitySystemFound(UAbilitySystemComponent* FoundAbilitySystem)
{
	DestroyAbilitySystemFinder();

	if (WeakASC != FoundAbilitySystem)
	{
		UnlinkAbilitySystem();
		WeakASC = FoundAbilitySystem;
	}
}

void UOverdriveCombatComponent::HandleAbilitySystemFindFailed()
{
	DestroyAbilitySystemFinder();

	ensureMsgf(false, TEXT("%hs::Can't Find AbilitySystem."), __FUNCTION__);
}

void UOverdriveCombatComponent::DestroyAbilitySystemFinder()
{
	if (!IsValid(AbilitySystemFinder))
	{
		return;
	}

	AbilitySystemFinder->StopFind();
	AbilitySystemFinder->MarkAsGarbage();
	AbilitySystemFinder = nullptr;
}

void UOverdriveCombatComponent::UnlinkAbilitySystem()
{
	if(WeakASC.IsValid())
	{
		WeakASC.Reset();
	}
}



void UOverdriveCombatComponent::ApplyHitStop(float NewTime)
{
	if (NewTime <= 0.0f || !WeakASC.IsValid() || !HitStopEffect)
	{
		return;
	}

	// 연타 시 이전 히트스톱 GE 가 잔존하지 않도록 먼저 걷어낸다.
	ClearHitStop();

	FGameplayEffectContextHandle ContextHandle = WeakASC->MakeEffectContext();

	FGameplayEffectSpec HitStopEffectSpec(HitStopEffect.GetDefaultObject(), ContextHandle);

	HitStopEffectSpec.SetDuration(NewTime, true);

	HitStopEffectHandle = WeakASC->ApplyGameplayEffectSpecToSelf(HitStopEffectSpec);
}

void UOverdriveCombatComponent::ClearHitStop()
{
	if (!HitStopEffectHandle.IsValid() || !WeakASC.IsValid())
	{
		return;
	}

	WeakASC->RemoveActiveGameplayEffect(HitStopEffectHandle);
	HitStopEffectHandle.Invalidate();
}

void UOverdriveCombatComponent::CheckDamageApplier()
{
	OptionalDamageAppliers.RemoveAll([](const TWeakObjectPtr<UOverdriveCombatDamageApplier>& E) { return !E.IsValid(); });
}

void UOverdriveCombatComponent::AddOptionalDamageApplier(UOverdriveCombatDamageApplier* NewApplier)
{
	if (!NewApplier)
	{
		return;
	}

	NewApplier->LinkCombatComponent(this);

	// 중복 등록되면 ApplyDamage 파이프라인에서 같은 Applier 가 두 번 돈다.
	// 제거는 Remove 가 일치 항목을 전부 걷어내므로 중복을 참조 카운트로 쓸 수도 없다.
	OptionalDamageAppliers.AddUnique(NewApplier);
}

void UOverdriveCombatComponent::RemoveOptionalDamageApplier(UOverdriveCombatDamageApplier* InApplier)
{
	if (!InApplier || InApplier->GetLinkedCombatComponent() != this)
	{
		return;
	}

	InApplier->UnlinkCombatComponent();
	OptionalDamageAppliers.Remove(InApplier);
}

void UOverdriveCombatComponent::ApplyDamage(float InDamage, const FGameplayEffectSpec& Spec)
{
	float RemainDamage = InDamage;

	bool bNeedToCheckApplier = false;

	// Applier 가 처리 중 자신을 목록에서 제거할 수 있으므로(배리어 파괴 → RemoveBarrier
	// → RemoveOptionalDamageApplier) 원본이 아니라 스냅샷을 순회한다.
	const TArray<TWeakObjectPtr<UOverdriveCombatDamageApplier>> AppliersSnapshot = OptionalDamageAppliers;
	for (const TWeakObjectPtr<UOverdriveCombatDamageApplier>& DamageApplier : AppliersSnapshot)
	{
		if (!DamageApplier.IsValid())
		{
			bNeedToCheckApplier = true;
			continue;
		}

		DamageApplier->ApplyDamage(RemainDamage, Spec);
	}

	if (ensureMsgf(DefaultDamageApplier != nullptr, TEXT("[OverdriveCombat] %s: DefaultDamageApplier 가 설정되지 않아 데미지 %.1f 이 소실됩니다."), *GetNameSafe(GetOwner()), RemainDamage))
	{
		DefaultDamageApplier->ApplyDamage(RemainDamage, Spec);
	}

	if (bNeedToCheckApplier)
	{
		CheckDamageApplier();
	}
}

void UOverdriveCombatComponent::AddDamageExtender(TSubclassOf<UOverdriveCombatDamageExtender> NewExtender)
{
	if (!NewExtender)
	{
		return;
	}

	DamageExtenders.Add(NewExtender);
}

void UOverdriveCombatComponent::RemoveDamageExtender(TSubclassOf<UOverdriveCombatDamageExtender> InExtender)
{
	if (!InExtender)
	{
		return;
	}

	DamageExtenders.Remove(InExtender);
}

void UOverdriveCombatComponent::RegisterBarrier(UOverdriveCombatBarrier* InBarrier)
{
	if (InBarrier != nullptr)
	{
		ActiveBarriers.AddUnique(InBarrier);
	}
}

void UOverdriveCombatComponent::UnregisterBarrier(UOverdriveCombatBarrier* InBarrier)
{
	ActiveBarriers.Remove(InBarrier);
}

void UOverdriveCombatComponent::ExtendDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	for (const TSubclassOf<UOverdriveCombatDamageExtender>& DamageExtenderClass : DamageExtenders)
	{
		if (!DamageExtenderClass)
		{
			continue;
		}

		const UOverdriveCombatDamageExtender* ExtenderCDO = DamageExtenderClass->GetDefaultObject<UOverdriveCombatDamageExtender>();
		ExtenderCDO->ExtendDamage(ExecutionParams, OutExecutionOutput);
	}
}


