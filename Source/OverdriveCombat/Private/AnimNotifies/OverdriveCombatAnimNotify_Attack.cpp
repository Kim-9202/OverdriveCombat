// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatAnimNotify_Attack.h"
#include "AnimNotifies/OverdriveCombatHitBurstDetector.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "OverdriveCombatTags.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "OverdriveCombatAnimNotify_Attack"

UOverdriveCombatAnimNotify_Attack::UOverdriveCombatAnimNotify_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EventTag = OverdriveCombatTags::Combat_Event_Hit;
}

void UOverdriveCombatAnimNotify_Attack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	// 디텍터 미설정은 데이터 실수라 발화마다 어설션을 띄우지 않는다. ValidateAssociatedAssets 가 로드·저장 때 경고한다.
	if (MeshComp == nullptr || HitDetector == nullptr)
	{
		return;
	}

	const UWorld* World = MeshComp->GetWorld();
	AActor* InstigatorActor = MeshComp->GetOwner();
	if (World == nullptr || InstigatorActor == nullptr)
	{
		return;
	}

	// 감지 전에 월드 원점을 심는다. 같은 컴포넌트 중복 시 최근접 히트 선택과 ImpactContext 가 이 값을 공유한다.
	const FTransform ComponentToWorld = MeshComp->GetComponentTransform();

	FOverdriveCombatHitBurstDetectorContext Context;
	Context.MeshComp = MeshComp;
	Context.OwnerActor = InstigatorActor;
	Context.WorldOrigin = ComponentToWorld.TransformPosition(AttackOrigin);

#if WITH_EDITOR
	// 에디터 프리뷰에서는 물리 판정 없이 셰이프만 디버그 드로우한다.
	if (World->WorldType == EWorldType::EditorPreview)
	{
		HitDetector->DrawDebugDetection(Context);
		return;
	}
#endif

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	if (HitDetector->DetectHit(Context, TargetDataHandle) <= 0)
	{
		return;
	}

	// 전송 전에 Origin 을 심고, 스펙 규칙에 따라 ImpactNormal 을 재계산한다.
	FOverdriveCombatImpactContext ImpactContext;
	ImpactContext.WorldOrigin = Context.WorldOrigin;
	ImpactContext.ComponentToWorld = ComponentToWorld;
	OverdriveCombatHitEvents::ApplyImpactPostProcess(TargetDataHandle, ImpactContext, ImpactNormalSpec);

	// 노티파이 인스턴스는 애님 애셋당 하나이고 여러 메시가 공유하므로, 태그가 비었어도 멤버를 고치지 않고 지역으로 대체한다.
	const FGameplayTag ResolvedEventTag = EventTag.IsValid() ? EventTag : OverdriveCombatTags::Combat_Event_Hit;

	FGameplayEventData Payload;
	Payload.EventTag = ResolvedEventTag;
	Payload.Instigator = InstigatorActor;

	Payload.TargetData = TargetDataHandle;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InstigatorActor, ResolvedEventTag, Payload);
}

FString UOverdriveCombatAnimNotify_Attack::GetNotifyName_Implementation() const
{
	if (HitDetector != nullptr)
	{
		return FString::Printf(TEXT("Attack: %s    "), *HitDetector->GetDetectorDisplayName());
	}

	return TEXT("Attack (No Detector)    ");
}

#if WITH_EDITOR
FName UOverdriveCombatAnimNotify_Attack::GetAttackOriginPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatAnimNotify_Attack, AttackOrigin);
}

FName UOverdriveCombatAnimNotify_Attack::GetImpactNormalSpecPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatAnimNotify_Attack, ImpactNormalSpec);
}

bool UOverdriveCombatAnimNotify_Attack::CanBePlaced(UAnimSequenceBase* Animation) const
{
	// GameplayEvent 는 어빌리티가 재생 중인 몽타주와 짝지어질 때만 의미가 있다(AnimNotify_GameplayCue 관례).
	return Animation != nullptr && Animation->IsA(UAnimMontage::StaticClass());
}

void UOverdriveCombatAnimNotify_Attack::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();

	static const FName NAME_AssetCheck("AssetCheck");

	UObject* ContainingAsset = GetContainingAsset();
	if (ContainingAsset == nullptr || HitDetector != nullptr)
	{
		return;
	}

	FMessageLog AssetCheckLog(NAME_AssetCheck);

	const FText Message = FText::Format(
		LOCTEXT("MissingHitDetector", "{0} 의 Attack 노티파이에 HitDetector 가 지정되지 않았습니다."),
		FText::AsCultureInvariant(GetNameSafe(ContainingAsset)));

	// 애셋 토큰을 붙이면 로그 항목을 눌러 해당 몽타주로 바로 이동할 수 있다.
	AssetCheckLog.Warning()
		->AddToken(FUObjectToken::Create(ContainingAsset))
		->AddToken(FTextToken::Create(Message));

	if (GIsEditor)
	{
		// 로드·저장 중에도 사용자가 놓치지 않도록 알림을 띄운다(엔진 노티파이 검증 관례).
		const bool bForce = true;
		AssetCheckLog.Notify(Message, EMessageSeverity::Warning, bForce);
	}
}
#endif

#undef LOCTEXT_NAMESPACE
