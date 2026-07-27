// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/OverdriveCombatAnimNotifyState_Attack.h"
#include "AnimNotifies/OverdriveCombatHitSweepDetector.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "OverdriveCombatTags.h"

#if WITH_EDITOR
#include "AnimPose.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "OverdriveCombatAnimNotifyState_Attack"

namespace
{
	/** 서브스텝 하한(초). 0 나눗셈·과밀 샘플을 막는다. */
	constexpr float MinSubStepTime = 0.005f;
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 런타임은 샘플링하지 않는다. 캐싱된 키프레임이 없으면(2개 미만) 스윕할 것이 없다.
	if (MeshComp == nullptr || HitDetector == nullptr || CachedKeyframes.Num() < 2)
	{
		return;
	}

	FInstanceRuntimeState& State = RuntimeStateMap.FindOrAdd(MeshComp);
	State.Elapsed = 0.0f;
	State.NextSegment = 0;
	State.AlreadyHitComponents.Reset();
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	FInstanceRuntimeState* State = RuntimeStateMap.Find(MeshComp);
	if (State == nullptr)
	{
		return;
	}

	State->Elapsed += FrameDeltaTime;

	const UWorld* World = MeshComp->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 에디터 프리뷰에는 ASC 가 없으므로 물리 판정·전송을 건너뛴다(디버그 드로우는 NotifyEnd).
	if (World->WorldType == EWorldType::EditorPreview)
	{
		return;
	}

	AActor* InstigatorActor = MeshComp->GetOwner();
	if (HitDetector == nullptr || InstigatorActor == nullptr)
	{
		return;
	}

	const int32 SegmentCount = CachedKeyframes.Num() - 1;
	const FTransform ComponentToWorld = MeshComp->GetComponentTransform();
	const FVector WorldOrigin = ComponentToWorld.TransformPosition(AttackOrigin);

	// 이번 틱에 다음 키프레임 Time 을 지난 세그먼트를 스윕한다. 여러 세그먼트가 같은 컴포넌트를 맞추면
	// 가장 빠른 히트가 아니라 Origin 최근접 히트를 이번 틱 최적맵에 모은다.
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult> TickBestHits;
	while (State->NextSegment < SegmentCount && State->Elapsed >= CachedKeyframes[State->NextSegment + 1].Time)
	{
		HitDetector->DetectHitForSegment(MeshComp, InstigatorActor, CachedKeyframes[State->NextSegment].Transform, CachedKeyframes[State->NextSegment + 1].Transform, ComponentToWorld, WorldOrigin, State->AlreadyHitComponents, TickBestHits);
		++State->NextSegment;
	}

	// 스윕 결과가 나오면 곧바로(틱당 1회) 전송한다.
	if (TickBestHits.Num() > 0)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		HitDetector->CommitTickHits(TickBestHits, State->AlreadyHitComponents, TargetDataHandle);

		// 전송 전에 Origin 을 심고, 스펙 규칙에 따라 ImpactNormal 을 재계산한다.
		FOverdriveCombatImpactContext ImpactContext;
		ImpactContext.WorldOrigin = WorldOrigin;
		ImpactContext.ComponentToWorld = ComponentToWorld;
		OverdriveCombatHitEvents::ApplyImpactPostProcess(TargetDataHandle, ImpactContext, ImpactNormalSpec);

		OverdriveCombatHitEvents::SendHitEvent(InstigatorActor, TargetDataHandle, EventTag);
	}
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp == nullptr)
	{
		return;
	}

	// Begin 이 없었으면(엔트리 없음) 남은 세그먼트도 없다. 있으면 꺼내면서 맵에서 제거한다.
	FInstanceRuntimeState RemovedState;
	if (!RuntimeStateMap.RemoveAndCopyValue(MeshComp, RemovedState))
	{
		return;
	}

	const UWorld* World = MeshComp->GetWorld();
	if (HitDetector == nullptr || World == nullptr || CachedKeyframes.Num() < 2)
	{
		return;
	}

	const FTransform ComponentToWorld = MeshComp->GetComponentTransform();

	// 에디터 프리뷰에서는 물리 판정 없이 캐싱 궤적만 디버그 드로우한다.
	if (World->WorldType == EWorldType::EditorPreview)
	{
#if ENABLE_DRAW_DEBUG
		// 디텍터는 트랜스폼 배열을 받으므로 키프레임에서 트랜스폼만 뽑아 넘긴다.
		TArray<FTransform> DebugXforms;
		DebugXforms.Reserve(CachedKeyframes.Num());
		for (const FOverdriveCombatAttackKeyframe& Keyframe : CachedKeyframes)
		{
			DebugXforms.Add(Keyframe.Transform);
		}
		HitDetector->DrawDebugSweep(World, DebugXforms, ComponentToWorld);
#endif
		return;
	}

	AActor* InstigatorActor = MeshComp->GetOwner();
	if (InstigatorActor == nullptr)
	{
		return;
	}

	// 저프레임 등으로 틱에서 처리하지 못한 남은 세그먼트를 마저 스윕한다.
	const int32 SegmentCount = CachedKeyframes.Num() - 1;
	const FVector WorldOrigin = ComponentToWorld.TransformPosition(AttackOrigin);
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult> TickBestHits;
	while (RemovedState.NextSegment < SegmentCount)
	{
		HitDetector->DetectHitForSegment(MeshComp, InstigatorActor, CachedKeyframes[RemovedState.NextSegment].Transform, CachedKeyframes[RemovedState.NextSegment + 1].Transform, ComponentToWorld, WorldOrigin, RemovedState.AlreadyHitComponents, TickBestHits);
		++RemovedState.NextSegment;
	}

	if (TickBestHits.Num() > 0)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		HitDetector->CommitTickHits(TickBestHits, RemovedState.AlreadyHitComponents, TargetDataHandle);

		// 전송 전에 Origin 을 심고, 스펙 규칙에 따라 ImpactNormal 을 재계산한다.
		FOverdriveCombatImpactContext ImpactContext;
		ImpactContext.WorldOrigin = WorldOrigin;
		ImpactContext.ComponentToWorld = ComponentToWorld;
		OverdriveCombatHitEvents::ApplyImpactPostProcess(TargetDataHandle, ImpactContext, ImpactNormalSpec);

		OverdriveCombatHitEvents::SendHitEvent(InstigatorActor, TargetDataHandle, EventTag);
	}
}

FString UOverdriveCombatAnimNotifyState_Attack::GetNotifyName_Implementation() const
{
	if (HitDetector != nullptr)
	{
#if WITH_EDITOR
		return FString::Printf(TEXT("Attack Sweep: %s (%d seg)"), *HitDetector->GetDetectorDisplayName(), FMath::Max(0, CachedKeyframes.Num() - 1));
#else
		return TEXT("Attack Sweep");
#endif
	}

	return TEXT("Attack Sweep (No Detector)");
}

#if WITH_EDITOR
FName UOverdriveCombatAnimNotifyState_Attack::GetAttackOriginPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatAnimNotifyState_Attack, AttackOrigin);
}

FName UOverdriveCombatAnimNotifyState_Attack::GetImpactNormalSpecPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UOverdriveCombatAnimNotifyState_Attack, ImpactNormalSpec);
}

bool UOverdriveCombatAnimNotifyState_Attack::TryGetNotifyWindow(const UAnimMontage* Montage, float& OutStartTime, float& OutDuration) const
{
	if (Montage == nullptr)
	{
		return false;
	}

	// 이 노티파이 스테이트 인스턴스가 배치된 이벤트의 구간(시작·길이)을 찾는다.
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyStateClass == this)
		{
			OutStartTime = Event.GetTriggerTime();
			OutDuration = Event.GetDuration();
			return true;
		}
	}

	return false;
}

void UOverdriveCombatAnimNotifyState_Attack::CacheAttackKeyframes()
{
	UAnimMontage* Montage = GetTypedOuter<UAnimMontage>();
	if (Montage == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OverdriveCombat] CacheAttackKeyframes: 몽타주 아웃터를 찾지 못했습니다."));
		return;
	}

	if (HitDetector == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OverdriveCombat] CacheAttackKeyframes: HitDetector 가 없습니다."));
		return;
	}

	float StartTime = 0.0f;
	float WindowDuration = 0.0f;
	if (!TryGetNotifyWindow(Montage, StartTime, WindowDuration))
	{
		UE_LOG(LogTemp, Warning, TEXT("[OverdriveCombat] CacheAttackKeyframes: 노티파이 이벤트를 몽타주에서 찾지 못했습니다."));
		return;
	}

	// 소켓 해석·리타깃 비율용 프리뷰 메시(없어도 스켈레톤 본으로 동작).
	USkeletalMesh* Mesh = Montage->GetPreviewMesh();
	if (Mesh == nullptr)
	{
		if (USkeleton* Skeleton = Montage->GetSkeleton())
		{
			Mesh = Skeleton->GetPreviewMesh(true);
		}
	}

	// 구간을 덮는 슬롯 트랙을 고른다(대개 단일 공격 슬롯). 없으면 첫 슬롯으로 폴백.
	const FAnimTrack* Track = nullptr;
	for (const FSlotAnimationTrack& Slot : Montage->SlotAnimTracks)
	{
		if (Slot.AnimTrack.GetSegmentAtTime(StartTime) != nullptr)
		{
			Track = &Slot.AnimTrack;
			break;
		}
	}

	if (Track == nullptr && Montage->SlotAnimTracks.Num() > 0)
	{
		Track = &Montage->SlotAnimTracks[0].AnimTrack;
	}

	if (Track == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OverdriveCombat] CacheAttackKeyframes: 슬롯 트랙이 없습니다."));
		return;
	}

	const FName Anchor = HitDetector->GetSocketName();
	const float SafeStep = FMath::Max(SubStepTime, MinSubStepTime);
	const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(WindowDuration / SafeStep));
	const float MontageLength = Montage->GetPlayLength();

	FAnimPoseEvaluationOptions Options;
	Options.EvaluationType = EAnimDataEvalType::Raw;
	Options.bShouldRetarget = true;
	Options.bExtractRootMotion = false;
	Options.OptionalSkeletalMesh = Mesh;

	Modify();
	CachedKeyframes.Reset();
	CachedKeyframes.Reserve(SegmentCount + 1);

	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SegmentCount);
		float TrackTime = StartTime + Alpha * WindowDuration;
		// 트랙 경계를 벗어나면 세그먼트 조회가 실패하므로 몽타주 길이 안쪽으로 클램프한다.
		TrackTime = FMath::Clamp(TrackTime, 0.0f, FMath::Max(0.0f, MontageLength - KINDA_SMALL_NUMBER));

		FTransform CompSpace = FTransform::Identity;
		if (const FAnimSegment* Segment = Track->GetSegmentAtTime(TrackTime))
		{
			float PositionInAnim = 0.0f;
			UAnimSequenceBase* Sequence = Segment->GetAnimationData(TrackTime, PositionInAnim);

			// 몽타주는 GetAnimationPose 가 check(false) 이므로 하위 UAnimSequence 에서만 평가한다.
			if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(Sequence))
			{
				FAnimPose Pose;
				UAnimPoseExtensions::GetAnimPoseAtTime(AnimSequence, PositionInAnim, Options, Pose);
				CompSpace = ResolveAnchorComponentSpace(Pose, Anchor, Mesh);
			}
		}

		FOverdriveCombatAttackKeyframe& Keyframe = CachedKeyframes.AddDefaulted_GetRef();
		Keyframe.Transform = CompSpace;
		Keyframe.Time = Alpha * WindowDuration;
	}

	CachedTotalDuration = WindowDuration;
	Montage->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[OverdriveCombat] CacheAttackKeyframes: %d 세그먼트(%d 키프레임) 캐싱 완료."), FMath::Max(0, CachedKeyframes.Num() - 1), CachedKeyframes.Num());
}

FTransform UOverdriveCombatAnimNotifyState_Attack::ResolveAnchorComponentSpace(const FAnimPose& Pose, FName AnchorName, const USkeletalMesh* Mesh) const
{
	// 소켓 이름이 없으면 컴포넌트 루트(런타임 GetSocketTransform 의 NAME_None 과 동일).
	if (AnchorName.IsNone())
	{
		return FTransform::Identity;
	}

	// EAnimPoseSpaces::World == 컴포넌트 공간(엔진 enum 주석). FindSocket 는 메시·스켈레톤 소켓을 모두 조회한다.
	const bool bIsSocket = (Mesh != nullptr) && (Mesh->FindSocket(AnchorName) != nullptr);
	if (bIsSocket)
	{
		return UAnimPoseExtensions::GetSocketPose(Pose, AnchorName, EAnimPoseSpaces::World);
	}

	return UAnimPoseExtensions::GetBonePose(Pose, AnchorName, EAnimPoseSpaces::World);
}

bool UOverdriveCombatAnimNotifyState_Attack::CanBePlaced(UAnimSequenceBase* Animation) const
{
	// GameplayEvent 는 어빌리티가 재생 중인 몽타주와 짝지어질 때만 의미가 있다(AnimNotify_GameplayCue 관례).
	return Animation != nullptr && Animation->IsA(UAnimMontage::StaticClass());
}

EDataValidationResult UOverdriveCombatAnimNotifyState_Attack::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (HitDetector == nullptr)
	{
		Context.AddError(LOCTEXT("MissingHitDetector", "Combat Attack Sweep 노티파이에 HitDetector 가 지정되지 않았습니다."));
		Result = EDataValidationResult::Invalid;
	}

	if (CachedKeyframes.Num() < 2)
	{
		Context.AddError(LOCTEXT("MissingCachedKeyframes", "키프레임이 캐싱되지 않았습니다. 디테일 패널의 Cache 버튼을 눌러 베이크하세요."));
		Result = EDataValidationResult::Invalid;
	}
	else
	{
		// 노티파이 길이가 베이크 이후 바뀌었으면 재베이크가 필요하다.
		float StartTime = 0.0f;
		float WindowDuration = 0.0f;
		if (TryGetNotifyWindow(GetTypedOuter<UAnimMontage>(), StartTime, WindowDuration) && !FMath::IsNearlyEqual(CachedTotalDuration, WindowDuration))
		{
			Context.AddWarning(LOCTEXT("StaleCachedKeyframes", "노티파이 길이가 마지막 베이크 이후 변경되었습니다. Cache 버튼으로 다시 베이크하세요."));
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
