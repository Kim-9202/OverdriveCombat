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
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "OverdriveCombatAnimNotifyState_Attack"

namespace
{
	/** 서브스텝 하한(초). 0 나눗셈·과밀 샘플을 막는다. */
	constexpr float MinSubStepTime = 0.005f;

	/** 캐시 구간 스테일 판정 허용 오차(초). 프레임 단위보다 훨씬 작게 잡아 실제 이동만 잡는다. */
	constexpr float StaleWindowTolerance = 1.0e-3f;
}

UOverdriveCombatAnimNotifyState_Attack::UOverdriveCombatAnimNotifyState_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EventTag = OverdriveCombatTags::Combat_Event_Hit;
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	// 런타임은 샘플링하지 않는다. 캐싱된 키프레임이 없으면(2개 미만) 스윕할 것이 없다.
	if (MeshComp == nullptr || HitDetector == nullptr || CachedKeyframes.Num() < 2)
	{
		return;
	}

	FInstanceRuntimeState& State = RuntimeStateMap.FindOrAdd(MeshComp);
	State.NextSegment = 0;
	State.AlreadyHitComponents.Reset();
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp == nullptr)
	{
		return;
	}

	FInstanceRuntimeState* State = RuntimeStateMap.Find(MeshComp);
	if (State == nullptr)
	{
		return;
	}

	// 몽타주 트랙 시간을 그대로 쓴다. FrameDeltaTime 은 PlayRate·블렌드·스크럽을 반영하지 않아 궤적과 어긋난다.
	// 이번 틱에 다음 키프레임 Time 을 지난 세그먼트만 처리한다(스윕 결과는 틱당 1회 전송).
	ProcessDueSegments(MeshComp, *State, EventReference.GetCurrentAnimationTime());
}

void UOverdriveCombatAnimNotifyState_Attack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
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

	// 마지막으로 알려진 애니메이션 시간까지만 처리한다. 몽타주가 중간에 끊기면 지나지 않은 구간은 판정하지 않는다.
	// 여기 오는 참조는 노티파이가 마지막으로 살아 있던 틱의 값이고, 시간을 못 얻는 경로에서는 0 이라 아무것도 스윕하지 않는다.
	ProcessDueSegments(MeshComp, RemovedState, EventReference.GetCurrentAnimationTime());
}

int32 UOverdriveCombatAnimNotifyState_Attack::FindDueSegmentEnd(float AnimTimeLimit, int32 FromSegment) const
{
	const int32 SegmentCount = CachedKeyframes.Num() - 1;

	// 되감겨도 이미 처리한 세그먼트로 돌아가지 않도록 항상 진행 지점부터 스캔한다(단조 진행).
	int32 DueEnd = FMath::Clamp(FromSegment, 0, SegmentCount);
	while (DueEnd < SegmentCount && CachedKeyframes[DueEnd + 1].Time <= AnimTimeLimit)
	{
		++DueEnd;
	}

	return DueEnd;
}

#if WITH_EDITOR
void UOverdriveCombatAnimNotifyState_Attack::DrawPreviewSegments(const UWorld* World, const FTransform& ComponentToWorld, int32 FirstSegment, int32 EndSegment) const
{
	for (int32 Index = FirstSegment; Index < EndSegment; ++Index)
	{
		HitDetector->DrawDebugSweepSegment(World, CachedKeyframes[Index].Transform, CachedKeyframes[Index + 1].Transform, ComponentToWorld);
	}
}
#endif

void UOverdriveCombatAnimNotifyState_Attack::ProcessDueSegments(USkeletalMeshComponent* MeshComp, FInstanceRuntimeState& State, float AnimTimeLimit) const
{
	const UWorld* World = (MeshComp != nullptr) ? MeshComp->GetWorld() : nullptr;
	if (World == nullptr || HitDetector == nullptr || CachedKeyframes.Num() < 2)
	{
		return;
	}

	const int32 DueEnd = FindDueSegmentEnd(AnimTimeLimit, State.NextSegment);
	if (State.NextSegment >= DueEnd)
	{
		return;
	}

	const FTransform ComponentToWorld = MeshComp->GetComponentTransform();

#if WITH_EDITOR
	// 프리뷰 액터에는 ASC 도 판정 대상도 없다. 세그먼트 진행 규칙은 런타임과 같게 두고 판정·전송만 건너뛴다.
	if (World->WorldType == EWorldType::EditorPreview)
	{
		DrawPreviewSegments(World, ComponentToWorld, State.NextSegment, DueEnd);
		State.NextSegment = DueEnd;

		return;
	}
#endif

	AActor* InstigatorActor = MeshComp->GetOwner();
	if (InstigatorActor == nullptr)
	{
		return;
	}

	// 프리뷰 가드 뒤에 둔다 — 프리뷰 월드도 권위로 잡히므로 순서를 뒤집으면 프리뷰 드로우가 정책에 걸린다.
	// 건너뛸 때도 세그먼트를 소비 처리해야 매 틱 같은 구간을 다시 평가하지 않는다(위 프리뷰 분기와 같은 형태).
	if (!OverdriveCombatHitEvents::ShouldFireForNetPolicy(InstigatorActor, NetPolicy))
	{
		State.NextSegment = DueEnd;

		return;
	}

	const FVector WorldOrigin = ComponentToWorld.TransformPosition(AttackOrigin);

	// 여러 세그먼트가 같은 컴포넌트를 맞추면 가장 빠른 히트가 아니라 Origin 최근접 히트를 이번 묶음 최적맵에 모은다.
	TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult> TickBestHits;
	for (int32 Index = State.NextSegment; Index < DueEnd; ++Index)
	{
		HitDetector->DetectHitForSegment(MeshComp, InstigatorActor, CachedKeyframes[Index].Transform, CachedKeyframes[Index + 1].Transform, ComponentToWorld, WorldOrigin, State.AlreadyHitComponents, TickBestHits);
	}

	State.NextSegment = DueEnd;

	if (TickBestHits.Num() == 0)
	{
		return;
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	HitDetector->CommitTickHits(TickBestHits, State.AlreadyHitComponents, TargetDataHandle);

	// 전송 전에 Origin 을 심고, 스펙 규칙에 따라 ImpactNormal 을 재계산한다.
	FOverdriveCombatImpactContext ImpactContext;
	ImpactContext.WorldOrigin = WorldOrigin;
	ImpactContext.ComponentToWorld = ComponentToWorld;
	OverdriveCombatHitEvents::ApplyImpactPostProcess(TargetDataHandle, ImpactContext, ImpactNormalSpec);

	OverdriveCombatHitEvents::SendHitEvent(InstigatorActor, TargetDataHandle, EventTag);
}

FString UOverdriveCombatAnimNotifyState_Attack::GetNotifyName_Implementation() const
{
	if (HitDetector != nullptr)
	{
#if WITH_EDITOR
		return FString::Printf(TEXT("Attack Sweep: %s"), *HitDetector->GetDetectorDisplayName());
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

bool UOverdriveCombatAnimNotifyState_Attack::TryGetNotifyWindow(const UAnimMontage* Montage, float& OutStartTime, float& OutEndTime) const
{
	if (Montage == nullptr)
	{
		return false;
	}

	// 이 노티파이 스테이트 인스턴스가 배치된 이벤트의 구간(시작·끝 시간)을 찾는다.
	// 트리거 오프셋까지 반영된 값이라 런타임이 실제로 발화하는 구간과 일치한다.
	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyStateClass == this)
		{
			OutStartTime = Event.GetTriggerTime();
			OutEndTime = Event.GetEndTriggerTime();
			return true;
		}
	}

	return false;
}

void UOverdriveCombatAnimNotifyState_Attack::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();

	static const FName NAME_AssetCheck("AssetCheck");

	UObject* ContainingAsset = GetContainingAsset();
	if (ContainingAsset == nullptr)
	{
		return;
	}

	const FText AssetName = FText::AsCultureInvariant(GetNameSafe(ContainingAsset));

	// 셋 다 이 노티파이가 판정을 못 하는 상태다. 더 근본적인 것부터 하나만 알린다.
	FText Message;
	if (HitDetector == nullptr)
	{
		Message = FText::Format(LOCTEXT("MissingHitDetector", "{0} 의 Attack Sweep 노티파이에 HitDetector 가 지정되지 않았습니다."), AssetName);
	}
	else if (CachedKeyframes.Num() < 2)
	{
		Message = FText::Format(LOCTEXT("MissingAttackKeyframes", "{0} 의 Attack Sweep 노티파이에 키프레임이 베이크되지 않았습니다. 디테일 패널의 Cache 버튼을 눌러 베이크하세요."), AssetName);
	}
	else
	{
		float StartTime = 0.0f;
		float EndTime = 0.0f;

		// 아웃터에서 이벤트를 못 찾으면(로드 도중 등) 구간은 판단하지 않는다.
		if (!TryGetNotifyWindow(GetTypedOuter<UAnimMontage>(), StartTime, EndTime))
		{
			return;
		}

		// 키프레임 Time 이 몽타주 절대 시간이라 노티파이를 옮기기만 해도 캐시가 어긋난다. 길이 변경도 같은 방식으로 잡힌다.
		if (FMath::IsNearlyEqual(CachedStartTime, StartTime, StaleWindowTolerance)
			&& FMath::IsNearlyEqual(CachedEndTime, EndTime, StaleWindowTolerance))
		{
			return;
		}

		Message = FText::Format(LOCTEXT("StaleAttackKeyframes", "{0} 의 Attack Sweep 구간이 마지막 베이크 이후 이동·변경되었습니다. Cache 버튼으로 다시 베이크하세요."), AssetName);
	}

	FMessageLog AssetCheckLog(NAME_AssetCheck);

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
	float EndTime = 0.0f;
	if (!TryGetNotifyWindow(Montage, StartTime, EndTime))
	{
		UE_LOG(LogTemp, Warning, TEXT("[OverdriveCombat] CacheAttackKeyframes: 노티파이 이벤트를 몽타주에서 찾지 못했습니다."));
		return;
	}

	const float WindowDuration = FMath::Max(0.0f, EndTime - StartTime);

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

	// 런타임은 루트모션을 캡슐로 빼고 포즈의 루트를 잠근다. 베이크가 루트를 안 잠그면 루트 변위가 앵커까지 전파돼 궤적에 이중으로 실린다.
	Options.bExtractRootMotion = true;
	Options.bIncorporateRootMotionIntoPose = false;
	Options.OptionalSkeletalMesh = Mesh;

	Modify();
	CachedKeyframes.Reset();
	CachedKeyframes.Reserve(SegmentCount + 1);

	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SegmentCount);
		const float KeyframeTime = StartTime + Alpha * WindowDuration;
		// 포즈 샘플만 트랙 경계 안쪽으로 클램프한다. 경계를 벗어나면 세그먼트 조회가 실패한다.
		const float SampleTime = FMath::Clamp(KeyframeTime, 0.0f, FMath::Max(0.0f, MontageLength - KINDA_SMALL_NUMBER));

		FTransform CompSpace = FTransform::Identity;
		if (const FAnimSegment* Segment = Track->GetSegmentAtTime(SampleTime))
		{
			float PositionInAnim = 0.0f;
			UAnimSequenceBase* Sequence = Segment->GetAnimationData(SampleTime, PositionInAnim);

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
		// 런타임이 애니메이션 시간과 직접 비교하도록 몽타주 트랙 시간을 그대로 담는다.
		Keyframe.Time = KeyframeTime;
	}

	// 베이크한 구간을 남겨 둔다. 이후 노티파이를 옮기거나 길이를 바꾸면 ValidateAssociatedAssets 가 이 값과 비교해 경고한다.
	CachedStartTime = StartTime;
	CachedEndTime = EndTime;

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

#endif

#undef LOCTEXT_NAMESPACE
