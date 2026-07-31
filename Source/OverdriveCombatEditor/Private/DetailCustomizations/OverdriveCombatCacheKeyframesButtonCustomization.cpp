// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailCustomizations/OverdriveCombatCacheKeyframesButtonCustomization.h"

#include "AnimNotifies/OverdriveCombatAnimNotifyState_Attack.h"

#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "OverdriveCombatCacheKeyframesButton"

TSharedRef<IPropertyTypeCustomization> FOverdriveCombatCacheKeyframesButtonCustomization::MakeInstance()
{
	return MakeShared<FOverdriveCombatCacheKeyframesButtonCustomization>();
}

void FOverdriveCombatCacheKeyframesButtonCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;

	HeaderRow
	.WholeRowContent()
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.Text(LOCTEXT("CacheButtonText", "Cache Attack Keyframes"))
		.ToolTipText(LOCTEXT("CacheButtonTooltip", "노티파이 구간을 Sub Step Time 단위로 샘플링해 앵커 궤적을 베이크한다."))
		.IsEnabled(this, &FOverdriveCombatCacheKeyframesButtonCustomization::IsCacheEnabled)
		.OnClicked(this, &FOverdriveCombatCacheKeyframesButtonCustomization::HandleCacheClicked)
	];
}

void FOverdriveCombatCacheKeyframesButtonCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// 값이 없는 자리표시자라 자식 행이 없다.
}

void FOverdriveCombatCacheKeyframesButtonCustomization::GatherNotifies(TArray<UOverdriveCombatAnimNotifyState_Attack*>& OutNotifies) const
{
	if (!StructPropertyHandle.IsValid() || !StructPropertyHandle->IsValidHandle())
	{
		return;
	}

	// Instanced 서브오브젝트의 프로퍼티라 핸들의 아우터가 곧 노티파이 인스턴스다.
	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	for (UObject* Outer : OuterObjects)
	{
		if (UOverdriveCombatAnimNotifyState_Attack* Notify = Cast<UOverdriveCombatAnimNotifyState_Attack>(Outer))
		{
			OutNotifies.Add(Notify);
		}
	}
}

FReply FOverdriveCombatCacheKeyframesButtonCustomization::HandleCacheClicked() const
{
	TArray<UOverdriveCombatAnimNotifyState_Attack*> Notifies;
	GatherNotifies(Notifies);

	if (Notifies.IsEmpty())
	{
		return FReply::Handled();
	}

	// CacheAttackKeyframes 가 Modify 와 MarkPackageDirty 를 부르므로 여기서는 트랜잭션만 연다.
	const FScopedTransaction Transaction(LOCTEXT("CacheKeyframesTransaction", "Cache Attack Keyframes"));

	for (UOverdriveCombatAnimNotifyState_Attack* Notify : Notifies)
	{
		Notify->CacheAttackKeyframes();
	}

	return FReply::Handled();
}

bool FOverdriveCombatCacheKeyframesButtonCustomization::IsCacheEnabled() const
{
	TArray<UOverdriveCombatAnimNotifyState_Attack*> Notifies;
	GatherNotifies(Notifies);

	return !Notifies.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
