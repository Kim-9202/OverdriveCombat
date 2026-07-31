// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"

class UOverdriveCombatAnimNotifyState_Attack;

/** FOverdriveCombatCacheKeyframesButton 자리에 CacheAttackKeyframes 실행 버튼을 그린다. */
class FOverdriveCombatCacheKeyframesButtonCustomization : public IPropertyTypeCustomization
{
public:
	/** 프로퍼티 타입 레이아웃 등록에 쓰는 팩토리. */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** 이 프로퍼티를 소유한 노티파이들을 모은다. */
	void GatherNotifies(TArray<UOverdriveCombatAnimNotifyState_Attack*>& OutNotifies) const;

	/** 버튼 클릭 처리. 선택된 노티파이마다 베이크를 실행한다. */
	FReply HandleCacheClicked() const;

	/** 유효한 노티파이가 하나라도 잡힐 때만 버튼을 활성화한다. */
	bool IsCacheEnabled() const;

	/** 아우터 노티파이를 되짚는 데 쓰는 자기 프로퍼티 핸들. */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
};
