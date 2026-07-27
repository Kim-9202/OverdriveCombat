// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class OverdriveCombatEditor : ModuleRules
{
	public OverdriveCombatEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				// FEdMode / EditorModeRegistry / GEditor / AssetEditorSubsystem / UEditorNotifyObject
				// + UBlueprintFactory / SClassPickerDialog / FKismetEditorUtilities
				"UnrealEd",
				// IAssetTools::RegisterAdvancedAssetCategory / FAssetToolsModule
				"AssetTools",
				// IClassViewerFilter / FClassViewerInitializationOptions / FClassViewerFilterFuncs
				"ClassViewer",
				// Tools/Modes.h (FEditorModeID)
				"EditorFramework",
				// IPersonaEditMode / IPersonaPreviewScene
				"Persona",
				// FAnimationEditMode (IPersonaEditMode 의 베이스)
				"AnimationEditMode",
				// IAnimationEditor (노티파이 선택 델리게이트)
				"AnimationEditor",
				"OverdriveCombat",
			}
			);
	}
}
