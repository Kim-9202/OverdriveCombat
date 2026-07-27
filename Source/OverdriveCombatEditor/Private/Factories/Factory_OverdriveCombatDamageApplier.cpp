// Fill out your copyright notice in the Description page of Project Settings.


#include "Factories/Factory_OverdriveCombatDamageApplier.h"

#include "OverdriveCombatEditorModule.h"
#include "OverdriveCombatDamageApplier.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "OverdriveCombatEditor"

namespace
{
	/**
	 * UOverdriveCombatDamageApplier 자신과 그 자식(네이티브 + 언로드 BP)만 통과시키는 클래스 뷰어 필터.
	 * 엔진 FBlueprintParentFilter(EditorFactories.cpp)를 allow-list 형태로 뒤집은 것.
	 */
	class FDamageApplierParentFilter : public IClassViewerFilter
	{
	public:
		/** 이 클래스이거나 그 자식만 허용한다. */
		TSet<const UClass*> AllowedChildrenOfClasses{ UOverdriveCombatDamageApplier::StaticClass() };

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (InClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				return false;
			}

			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) == EFilterReturn::Passed;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (InUnloadedClassData->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				return false;
			}

			return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) == EFilterReturn::Passed;
		}
	};
}

UFactory_OverdriveCombatDamageApplier::UFactory_OverdriveCombatDamageApplier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 베이스(UBlueprintFactory)가 SupportedClass=UBlueprint, bCreateNew/bEditAfterNew=true, ParentClass=AActor 로 세팅한다.
	// 기본 부모만 DamageApplier 로 덮어써 둔다(실제 선택은 ConfigureProperties 의 피커에서).
	ParentClass = UOverdriveCombatDamageApplier::StaticClass();
}

FText UFactory_OverdriveCombatDamageApplier::GetDisplayName() const
{
	return LOCTEXT("Factory_DamageApplier_DisplayName", "Damage Applier");
}

uint32 UFactory_OverdriveCombatDamageApplier::GetMenuCategories() const
{
	return FOverdriveCombatEditorModule::GetAssetCategoryBit();
}

const TArray<FText>& UFactory_OverdriveCombatDamageApplier::GetMenuCategorySubMenus() const
{
	static const TArray<FText> SubMenus{ LOCTEXT("Factory_DamageApplier_SubMenu", "Overdrive Combat") };
	return SubMenus;
}

bool UFactory_OverdriveCombatDamageApplier::ConfigureProperties()
{
	// 베이스 피커(트리뷰 / 언로드 BP 표시 / bIsBlueprintBaseOnly / UBlueprint 루트)를 그대로 쓰되,
	// 베이스가 Options 에 대해 호출하는 델리게이트(EditorFactories.cpp)에 DamageApplier 필터만 얹는다.
	OnConfigurePropertiesDelegate.BindLambda([](FClassViewerInitializationOptions* Options)
	{
		Options->bShowObjectRootClass = false;
		Options->bShowDefaultClasses = false;
		Options->ClassFilters.Add(MakeShared<FDamageApplierParentFilter>());
	});

	const bool bResult = Super::ConfigureProperties();

	OnConfigurePropertiesDelegate.Unbind();
	return bResult;
}

UObject* UFactory_OverdriveCombatDamageApplier::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// 피커 필터가 어떤 이유로든 뚫려도, DamageApplier 자식이 아닌 부모로는 생성하지 않는다(엔진 GA 팩토리 관례).
	if (ParentClass == nullptr || !ParentClass->IsChildOf(UOverdriveCombatDamageApplier::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != nullptr) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("NullParent", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("CannotCreateDamageApplier", "'{ClassName}' 은(는) DamageApplier 계열이 아니라서 Damage Applier 블루프린트를 만들 수 없습니다."), Args));
		return nullptr;
	}

	return Super::FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, CallingContext);
}

#undef LOCTEXT_NAMESPACE
