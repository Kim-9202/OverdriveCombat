// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class IAssetEditorInstance;

/**
 * OverdriveCombat 에디터 모듈.
 *
 * 히트 디텍터 기즈모 에디트 모드를 등록하고, 애니메이션 에디터의 노티파이 선택을 감지해
 * Attack 노티파이가 선택되면 에디트 모드를 활성화 / 비활성화한다.
 * 또한 콘텐츠 브라우저 생성 메뉴용 "Overdrive Plugins" 상위 카테고리를 등록한다.
 */
class FOverdriveCombatEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** 팩토리가 GetMenuCategories 에서 사용할 "Overdrive Plugins" 카테고리 비트. */
	static EAssetTypeCategories::Type GetAssetCategoryBit();

private:
	/** GEditor 준비 이후에만 UAssetEditorSubsystem 델리게이트를 바인딩할 수 있다. */
	void BindAssetEditorDelegates();

	/** 애니메이션 에디터가 열리면 선택 델리게이트를 바인딩한다. */
	void HandleAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance* Instance);

	/** 타임라인 선택이 바뀔 때마다 호출. Attack 노티파이면 에디트 모드를 켠다. */
	void HandleAnimationEditorObjectsSelected(const TArray<UObject*>& InObjects, IAssetEditorInstance* Instance);

	FDelegateHandle AssetOpenedHandle;

	/** RegisterAdvancedAssetCategory 로 할당받은 "Overdrive Plugins" 카테고리 비트. */
	static EAssetTypeCategories::Type OverdrivePluginsCategory;
};
