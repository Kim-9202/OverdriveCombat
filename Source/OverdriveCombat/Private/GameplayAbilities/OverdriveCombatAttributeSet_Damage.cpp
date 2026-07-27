// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/OverdriveCombatAttributeSet_Damage.h"
#include "GameplayEffectExtension.h"
#include "Components/OverdriveCombatComponent.h"
#include "OverdriveCombatTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilities/OverdriveCombatAttributeSet_Health.h"

void UOverdriveCombatAttributeSet_Damage::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetPendingDamageAttribute())
	{
		return;
	}

	// 위임 전에 소비한다 — 분배 도중 재진입이 일어나도 같은 데미지가 두 번 적용되지 않는다.
	// SetPendingDamage 는 SetNumericAttributeBase 를 타므로 BaseValue 까지 0 으로 돌아간다.
	const float Damage = GetPendingDamage();
	SetPendingDamage(0.0f);

	if (Damage <= 0.0f)
	{
		return;
	}

	AActor* AvatarActor = Data.Target.GetAvatarActor();
	if (AvatarActor == nullptr)
	{
		return;
	}

	UOverdriveCombatComponent* Combat = AvatarActor->FindComponentByClass<UOverdriveCombatComponent>();
	if (Combat == nullptr)
	{
		return;
	}

	Combat->ApplyDamage(Damage, Data.EffectSpec);

	PostDamageAction(Damage, Data);
}

void UOverdriveCombatAttributeSet_Damage::PostDamageAction(float InDamage, const FGameplayEffectModCallbackData& Data)
{
	const FGameplayEffectSpec& Spec = Data.EffectSpec;

	UAbilitySystemComponent* SourceASC = Spec.GetEffectContext().GetInstigatorAbilitySystemComponent();
	UAbilitySystemComponent& TargetASC = Data.Target;

	FGameplayEventData HitEventData;
	HitEventData.EventMagnitude = InDamage;
	HitEventData.ContextHandle = Spec.GetEffectContext();
	HitEventData.Target = TargetASC.GetAvatarActor();
	Spec.GetAllAssetTags(HitEventData.TargetTags);
	HitEventData.TargetTags.AppendTags(TargetASC.GetOwnedGameplayTags());

	// 가해자는 EffectContext 에서 얻는다 — 원격 클라이언트일 수 있으므로 여기서 도는 것은 서버 로직뿐이다.
	if (SourceASC)
	{
		HitEventData.Instigator = SourceASC->GetAvatarActor();
		HitEventData.InstigatorTags = SourceASC->GetOwnedGameplayTags();
	}

	if (SourceASC)
	{
		FGameplayEventData OnHitEventData = HitEventData;

		OnHitEventData.ContextHandle = OnHitEventData.ContextHandle.Duplicate();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceASC->GetOwnerActor(), OverdriveCombatTags::Combat_Event_OnHit, HitEventData);
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC.GetOwnerActor(), OverdriveCombatTags::Combat_Event_Damage, HitEventData);

	if (TargetASC.GetNumericAttribute(UOverdriveCombatAttributeSet_Health::GetHealthPointAttribute()) <= 0.f)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC.GetOwnerActor(), OverdriveCombatTags::Combat_Event_Dead, HitEventData);
	}
}
