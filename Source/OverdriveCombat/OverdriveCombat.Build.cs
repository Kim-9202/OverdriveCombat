// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OverdriveCombat : ModuleRules
{
	public OverdriveCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		// Public headers include AbilitySystemComponent.h, AttributeSet.h, GameplayTagContainer.h,
		// Engine/DeveloperSettings.h and Components/ActorComponent.h, so these must be public
		// dependencies for consumers to resolve the include paths.
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayTasks",
				"GameplayTags",
				"DeveloperSettings",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				// FVector_NetQuantize10::NetSerialize 가 쓰는 UE::Net::Write/ReadQuantizedVector 심볼(TargetData Origin).
				"NetCore"
				// ... add private dependencies that you statically link with here ...
			}
			);

		// 에디터 전용: Attack Sweep 노티파이의 CacheAttackKeyframes 가 UAnimPoseExtensions 로 몽타주 포즈를 베이크한다.
		// AnimationBlueprintLibrary 는 엔진 에디터 모듈이므로 에디터 빌드에서만 링크한다(런타임 코드는 #if WITH_EDITOR 가드).
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("AnimationBlueprintLibrary");
		}



		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
