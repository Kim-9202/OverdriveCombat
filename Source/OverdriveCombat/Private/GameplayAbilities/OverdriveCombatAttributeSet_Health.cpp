// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/OverdriveCombatAttributeSet_Health.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UOverdriveCombatAttributeSet_Health::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UOverdriveCombatAttributeSet_Health, MaxHealthPoint, COND_None, REPNOTIFY_OnChanged);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverdriveCombatAttributeSet_Health, HealthPoint, COND_None, REPNOTIFY_OnChanged);
}

void UOverdriveCombatAttributeSet_Health::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 체력은 항상 [0, MaxHealthPoint] 안에 있다. 최대치가 줄었을 때도 현재치를 따라 내린다.
	// 사망 판정은 게임 쪽이 HealthPoint 변경 델리게이트로 수행한다(플러그인은 범위 규칙만 강제).
	if (Data.EvaluatedData.Attribute == GetHealthPointAttribute() || Data.EvaluatedData.Attribute == GetMaxHealthPointAttribute())
	{
		SetHealthPoint(FMath::Clamp(GetHealthPoint(), 0.0f, GetMaxHealthPoint()));
	}
}

void UOverdriveCombatAttributeSet_Health::OnRep_MaxHealthPoint(const FGameplayAttributeData& OldData)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverdriveCombatAttributeSet_Health, MaxHealthPoint, OldData)

}

void UOverdriveCombatAttributeSet_Health::OnRep_HealthPoint(const FGameplayAttributeData& OldData)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverdriveCombatAttributeSet_Health, HealthPoint, OldData)

}


