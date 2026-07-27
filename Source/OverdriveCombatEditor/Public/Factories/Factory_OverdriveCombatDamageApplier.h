// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/BlueprintFactory.h"
#include "Factory_OverdriveCombatDamageApplier.generated.h"

/**
 * UOverdriveCombatDamageApplier 계열의 자식 블루프린트를 생성하는 팩토리.
 *
 * UBlueprintFactory 를 상속해 부모 클래스 피커와 CreateBlueprint 흐름을 그대로 재사용하고,
 * ConfigureProperties 에서 클래스 필터만 DamageApplier 계열로 제약한다.
 * 콘텐츠 브라우저 생성 메뉴의 "Overdrive Plugins > Overdrive Combat > Damage Applier" 에 나타난다.
 */
UCLASS()
class UFactory_OverdriveCombatDamageApplier : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UFactory_OverdriveCombatDamageApplier(const FObjectInitializer& ObjectInitializer);

	// UFactory 메뉴 배치
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual const TArray<FText>& GetMenuCategorySubMenus() const override;

	// UBlueprintFactory: 부모 클래스 피커를 DamageApplier 계열로 제약
	virtual bool ConfigureProperties() override;

	// 피커가 뚫려도 생성 시점에 부모가 DamageApplier 자식인지 재검증하는 하드 가드(엔진 GA 팩토리 관례).
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};
