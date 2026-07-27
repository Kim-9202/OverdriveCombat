// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatEditorModule.h"

#include "EditModes/OverdriveCombatHitBurstDetectorEditMode.h"
#include "EditModes/OverdriveCombatSweepDetectorEditMode.h"

#include "AnimNotifies/OverdriveCombatAnimNotify_Attack.h"
#include "AnimNotifies/OverdriveCombatAnimNotifyState_Attack.h"
#include "AnimNotifies/OverdriveCombatHitBurstDetector.h"
#include "AnimNotifies/OverdriveCombatHitSweepDetector.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/EditorNotifyObject.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorModeRegistry.h"
#include "IAnimationEditor.h"
#include "Misc/CoreDelegates.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "OverdriveCombatEditor"

namespace
{
	/** IAssetEditorInstance::GetEditorName 이 돌려주는 애니메이션 에디터 툴킷 이름. */
	const FName GAnimationEditorName(TEXT("AnimationEditor"));
}

EAssetTypeCategories::Type FOverdriveCombatEditorModule::OverdrivePluginsCategory = EAssetTypeCategories::Misc;

EAssetTypeCategories::Type FOverdriveCombatEditorModule::GetAssetCategoryBit()
{
	return OverdrivePluginsCategory;
}

void FOverdriveCombatEditorModule::StartupModule()
{
	// 콘텐츠 브라우저 생성 메뉴의 "Overdrive Plugins" 상위 카테고리 등록.
	// 키는 다른 Overdrive 플러그인(AssetDefinition 방식)이 쓰는 것과 반드시 같아야 병합된다.
	// AssetDefinition 은 FAssetCategoryPath 가 FName(*GetSourceString(FText)) 로 키를 만들므로
	// 표시 텍스트 "Overdrive Plugins" 그대로가 키가 된다(공백 포함). 여기서도 동일 FName 을 써야 한다.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	OverdrivePluginsCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("Overdrive Plugins"), LOCTEXT("OverdrivePluginsCategory", "Overdrive Plugins"));

	FEditorModeRegistry::Get().RegisterMode<FOverdriveCombatHitBurstDetectorEditMode>(
		FOverdriveCombatHitBurstDetectorEditMode::ModeID,
		LOCTEXT("HitDetectorEditModeName", "Overdrive Combat Hit Detector"),
		FSlateIcon(),
		false);

	FEditorModeRegistry::Get().RegisterMode<FOverdriveCombatSweepDetectorEditMode>(
		FOverdriveCombatSweepDetectorEditMode::ModeID,
		LOCTEXT("SweepDetectorEditModeName", "Overdrive Combat Sweep Detector"),
		FSlateIcon(),
		false);

	// 에디터 모듈은 GEditor 생성 전에 로드될 수 있다. 준비돼 있으면 즉시, 아니면 엔진 초기화 후에 바인딩한다.
	if (GEditor != nullptr)
	{
		BindAssetEditorDelegates();
	}
	else
	{
		FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FOverdriveCombatEditorModule::BindAssetEditorDelegates);
	}
}

void FOverdriveCombatEditorModule::ShutdownModule()
{
	FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);

	if (UObjectInitialized() && GEditor != nullptr)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OnAssetOpenedInEditor().Remove(AssetOpenedHandle);

			// 아직 열려 있는 애니메이션 에디터에 남은 선택 바인딩을 걷어낸다.
			for (UObject* Asset : AssetEditorSubsystem->GetAllEditedAssets())
			{
				IAssetEditorInstance* Instance = (Asset != nullptr) ? AssetEditorSubsystem->FindEditorForAsset(Asset, false) : nullptr;
				if (Instance != nullptr && Instance->GetEditorName() == GAnimationEditorName)
				{
					static_cast<IAnimationEditor*>(Instance)->OnAnimationEditorObjectsSelected().RemoveAll(this);
				}
			}
		}
	}

	FEditorModeRegistry::Get().UnregisterMode(FOverdriveCombatHitBurstDetectorEditMode::ModeID);
	FEditorModeRegistry::Get().UnregisterMode(FOverdriveCombatSweepDetectorEditMode::ModeID);
}

void FOverdriveCombatEditorModule::BindAssetEditorDelegates()
{
	UAssetEditorSubsystem* AssetEditorSubsystem = (GEditor != nullptr) ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
	if (AssetEditorSubsystem == nullptr)
	{
		return;
	}

	AssetOpenedHandle = AssetEditorSubsystem->OnAssetOpenedInEditor().AddRaw(this, &FOverdriveCombatEditorModule::HandleAssetOpenedInEditor);
}

void FOverdriveCombatEditorModule::HandleAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance* Instance)
{
	if (Instance == nullptr || Instance->GetEditorName() != GAnimationEditorName)
	{
		return;
	}

	if (Cast<UAnimSequenceBase>(Asset) == nullptr)
	{
		return;
	}

	// GetEditorName 으로 확인한 뒤의 다운캐스트는 엔진 관례다(AssetTypeActions_AnimationAsset 참고).
	IAnimationEditor* AnimationEditor = static_cast<IAnimationEditor*>(Instance);

	// 에셋 교체(Closed -> Opened)로 같은 에디터에 남은 이전 바인딩을 제거해 중복 호출을 막는다.
	// 에디터가 닫힐 때는 델리게이트가 에디터와 함께 소멸하므로 별도 해제가 필요 없다
	// (닫힘 이벤트는 소멸자에서 브로드캐스트되어 그 시점의 가상 함수 호출이 안전하지 않다).
	AnimationEditor->OnAnimationEditorObjectsSelected().RemoveAll(this);
	AnimationEditor->OnAnimationEditorObjectsSelected().AddRaw(this, &FOverdriveCombatEditorModule::HandleAnimationEditorObjectsSelected, Instance);
}

void FOverdriveCombatEditorModule::HandleAnimationEditorObjectsSelected(const TArray<UObject*>& InObjects, IAssetEditorInstance* Instance)
{
	// 이 콜백은 살아있는 에디터가 자기 멤버 델리게이트로 브로드캐스트할 때만 온다.
	IAnimationEditor* AnimationEditor = static_cast<IAnimationEditor*>(Instance);

	UOverdriveCombatAnimNotify_Attack* BurstNotify = nullptr;
	UOverdriveCombatAnimNotifyState_Attack* SweepNotify = nullptr;
	UOverdriveCombatHitBurstDetector* BurstDetector = nullptr;
	UOverdriveCombatHitSweepDetector* SweepDetector = nullptr;
	UAnimSequenceBase* AnimAsset = nullptr;

	for (UObject* Object : InObjects)
	{
		const UEditorNotifyObject* NotifyObject = Cast<UEditorNotifyObject>(Object);
		if (NotifyObject == nullptr)
		{
			continue;
		}

		// Event 는 사본이지만 Notify/NotifyStateClass 포인터는 애님 에셋이 소유한 실제 인스턴스를 가리킨다.
		// 선택된 노티파이는 단발 Notify 이거나 구간 NotifyState 중 하나다(둘 다는 아니다).
		BurstNotify = Cast<UOverdriveCombatAnimNotify_Attack>(NotifyObject->Event.Notify);
		SweepNotify = Cast<UOverdriveCombatAnimNotifyState_Attack>(NotifyObject->Event.NotifyStateClass);
		if (BurstNotify != nullptr || SweepNotify != nullptr)
		{
			BurstDetector = (BurstNotify != nullptr) ? BurstNotify->GetHitDetector() : nullptr;
			SweepDetector = (SweepNotify != nullptr) ? SweepNotify->GetHitDetector() : nullptr;
			AnimAsset = NotifyObject->AnimObject;
			break;
		}
	}

	FEditorModeTools& ModeTools = AnimationEditor->GetEditorModeManager();

	// 단발 에디트 모드: 노티파이가 선택돼 있으면 켠다(디텍터가 없어도 원점은 편집 가능).
	if (BurstNotify != nullptr)
	{
		ModeTools.ActivateMode(FOverdriveCombatHitBurstDetectorEditMode::ModeID);

		if (FEdMode* Mode = ModeTools.GetActiveMode(FOverdriveCombatHitBurstDetectorEditMode::ModeID))
		{
			static_cast<FOverdriveCombatHitBurstDetectorEditMode*>(Mode)->SetTarget(BurstDetector, BurstNotify, AnimAsset);
		}
	}
	else
	{
		if (FEdMode* Mode = ModeTools.GetActiveMode(FOverdriveCombatHitBurstDetectorEditMode::ModeID))
		{
			static_cast<FOverdriveCombatHitBurstDetectorEditMode*>(Mode)->ClearTarget();
		}

		ModeTools.DeactivateMode(FOverdriveCombatHitBurstDetectorEditMode::ModeID);
	}

	// 구간 스윕 에디트 모드: 위와 동일하게 독립적으로 관리한다.
	if (SweepNotify != nullptr)
	{
		ModeTools.ActivateMode(FOverdriveCombatSweepDetectorEditMode::ModeID);

		if (FEdMode* Mode = ModeTools.GetActiveMode(FOverdriveCombatSweepDetectorEditMode::ModeID))
		{
			static_cast<FOverdriveCombatSweepDetectorEditMode*>(Mode)->SetTarget(SweepDetector, SweepNotify, AnimAsset);
		}
	}
	else
	{
		if (FEdMode* Mode = ModeTools.GetActiveMode(FOverdriveCombatSweepDetectorEditMode::ModeID))
		{
			static_cast<FOverdriveCombatSweepDetectorEditMode*>(Mode)->ClearTarget();
		}

		ModeTools.DeactivateMode(FOverdriveCombatSweepDetectorEditMode::ModeID);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOverdriveCombatEditorModule, OverdriveCombatEditor)
