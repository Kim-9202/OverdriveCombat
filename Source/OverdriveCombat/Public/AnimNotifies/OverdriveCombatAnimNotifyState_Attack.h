// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "OverdriveCombatAnimNotifyState_Attack.generated.h"

class UOverdriveCombatHitSweepDetector;
class UPrimitiveComponent;
class UWorld;

/**
 * 앵커 궤적의 한 키프레임. 에디터에서 베이크해 직렬화하고, 런타임은 이 값을 그대로 스윕에 쓴다.
 */
USTRUCT()
struct FOverdriveCombatAttackKeyframe
{
	GENERATED_BODY()

	/** 앵커의 컴포넌트 상대 트랜스폼. 디텍터가 그대로 스윕에 쓴다. */
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	/** 이 키프레임을 실행할 몽타주 트랙 시간(초). 런타임은 CurrentAnimationTime 이 이 값을 지날 때 스윕한다. */
	UPROPERTY()
	float Time = 0.0f;
};

/** 디테일 패널의 Cache 버튼 자리표시자. 값은 없고, 에디터 모듈의 프로퍼티 타입 커스터마이제이션이 이 자리에 버튼을 그린다. */
USTRUCT()
struct FOverdriveCombatCacheKeyframesButton
{
	GENERATED_BODY()
};

/**
 * 구간 스윕 공격 판정 노티파이 스테이트.
 *
 * 앵커 소켓 궤적은 런타임이 아니라 에디터에서 미리 캐싱한다. 디테일 패널의 Cache 버튼(CacheAttackKeyframes)을 누르면
 * 노티파이 구간을 SubStepTime(초) 단위로 나눠, 각 시점의 소켓 컴포넌트 상대 트랜스폼과 몽타주 트랙 시간을 몽타주
 * 슬롯에서 평가해 CachedKeyframes 에 직렬화한다. 런타임은 샘플링 없이 이 캐시를 그대로 사용한다.
 *
 * 런타임에는 이벤트 참조가 알려주는 현재 애니메이션 시간(CurrentAnimationTime)이 각 키프레임의 Time 을 지날 때마다
 * 인접 키프레임을 현재 컴포넌트 트랜스폼으로 스윕하고, 그 틱에 나온 신규 히트를 모아 곧바로 GameplayEvent 로
 * 전송한다(틱당 1회). 몽타주 시간을 그대로 쓰므로 PlayRate·일시정지·에디터 스크럽까지 궤적과 동기화된다.
 * 시간이 되감겨도(섹션 루프·점프) 이미 처리한 세그먼트로 돌아가지 않는다.
 * 이전 스윕에서 이미 닿은 컴포넌트는 노티파이 구간 전체에 걸쳐 무시한다. 서버 권위에서만 이벤트를 보낸다.
 *
 * 컴포넌트 상대 공간으로 캐싱하므로 캐릭터 로코모션이 스윕 볼륨을 부풀리지 않는다.
 * 애님 에디터 프리뷰에서는 세그먼트 진행 규칙은 런타임과 같게 두되 물리 판정·이벤트 전송만 건너뛰고,
 * 지나간 세그먼트를 그때그때 디버그 드로우한다(od.Combat.DrawHitDetection).
 * 구간 전체 궤적은 노티파이를 선택했을 때 에디터 모듈의 에디트 모드가 그린다.
 *
 * 노티파이 인스턴스는 애님 애셋당 하나뿐이고 여러 메시가 공유하므로, 런타임 상태는 메시별로 보관한다.
 */
UCLASS(EditInlineNew, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "Overdrive Combat Attack (Sweep)"))
class OVERDRIVECOMBAT_API UOverdriveCombatAnimNotifyState_Attack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UOverdriveCombatAnimNotifyState_Attack(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** 에디터(기즈모 편집)가 쓰는 디텍터 접근자. */
	UOverdriveCombatHitSweepDetector* GetHitDetector() const { return HitDetector; }

	/** 컴포넌트 상대 공격 원점(런타임 후처리·기즈모 편집이 쓴다). */
	FVector GetAttackOrigin() const { return AttackOrigin; }

	/** ImpactNormal 재계산 스펙(런타임 후처리가 쓴다). */
	const FOverdriveCombatImpactNormalSpec& GetImpactNormalSpec() const { return ImpactNormalSpec; }

#if WITH_EDITOR
	/** 에디트 모드가 구간 전체 궤적을 그리기 위해 읽는 베이크 결과. */
	const TArray<FOverdriveCombatAttackKeyframe>& GetCachedKeyframes() const { return CachedKeyframes; }

	/** 기즈모가 드래그한 컴포넌트 상대 원점을 반영한다. */
	void SetAttackOrigin(const FVector& InLocal) { AttackOrigin = InLocal; }

	/** 기즈모 편집용 스펙 접근자(런타임 경로는 const getter 를 쓴다). */
	FOverdriveCombatImpactNormalSpec& GetImpactNormalSpecForEdit() { return ImpactNormalSpec; }

	/** AttackOrigin 이 private 이므로 에디터 모듈이 PostEditChangeProperty 에 쓰는 프로퍼티 이름 통로. */
	static FName GetAttackOriginPropertyName();

	/** ImpactNormalSpec 이 private 이므로 에디터 모듈이 PostEditChangeProperty 에 쓰는 프로퍼티 이름 통로. */
	static FName GetImpactNormalSpecPropertyName();

	/** 디테일 패널 버튼: 현재 노티파이 구간을 SubStepTime 단위로 샘플링해 CachedKeyframes 에 베이크한다. */
	UFUNCTION(CallInEditor, Category = "OverdriveCombat")
	void CacheAttackKeyframes();

	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;

	/** 로드·저장 시점의 캐시 점검 훅. 노티파이를 옮기거나 길이를 바꾼 뒤 재베이크를 잊으면 AssetCheck 메시지 로그로 경고한다. */
	virtual void ValidateAssociatedAssets() override;
#endif

private:
	/** 메시 인스턴스별 런타임 스윕 상태. 샘플링은 하지 않고 진행 세그먼트와 누적 중복만 추적한다. */
	struct FInstanceRuntimeState
	{
		/** 다음에 스윕할 세그먼트 인덱스(0 .. N-1). */
		int32 NextSegment = 0;

		/** 이미 닿은 컴포넌트. 노티파이 구간 전체에 걸쳐 중복 히트를 막는다. */
		TSet<TWeakObjectPtr<UPrimitiveComponent>> AlreadyHitComponents;
	};

	/**
	 * 아직 처리하지 않은 세그먼트 중 처리 시점이 된 것들을 한 묶음으로 처리한다.
	 * 틱과 구간 종료 모두 AnimTimeLimit 에 그 시점의 애니메이션 시간을 넘긴다.
	 *
	 * 게임 월드에서는 스윕 판정 후 이 묶음의 신규 히트를 한 번에 전송하고,
	 * 프리뷰 액터에는 ASC 도 판정 대상도 없으므로 에디터 프리뷰에서는 세그먼트 디버그 드로우만 한다.
	 */
	void ProcessDueSegments(USkeletalMeshComponent* MeshComp, FInstanceRuntimeState& State, float AnimTimeLimit) const;

	/**
	 * 처리 시점이 된 마지막 세그먼트의 다음 인덱스(FromSegment .. N). 다음 키프레임 Time 이 AnimTimeLimit 이하인 세그먼트까지가 대상이다.
	 * 항상 FromSegment 부터 스캔하므로 애니메이션 시간이 되감겨도 이미 처리한 세그먼트로 돌아가지 않는다.
	 * 프리뷰 드로우와 런타임 판정이 같은 진행 규칙을 쓰도록 이 한 곳에서만 판단한다.
	 */
	int32 FindDueSegmentEnd(float AnimTimeLimit, int32 FromSegment) const;

#if WITH_EDITOR
	/** [FirstSegment, EndSegment) 구간의 스윕 볼륨을 디버그 드로우한다. 애님 에디터 프리뷰 전용 경로다. */
	void DrawPreviewSegments(const UWorld* World, const FTransform& ComponentToWorld, int32 FirstSegment, int32 EndSegment) const;

	/** 캐싱된 포즈에서 앵커(소켓/본/루트)의 컴포넌트 상대 트랜스폼을 해석한다. 런타임 GetSocketTransform 와 동일 의미. */
	FTransform ResolveAnchorComponentSpace(const struct FAnimPose& Pose, FName AnchorName, const class USkeletalMesh* Mesh) const;

	/** 이 노티파이 인스턴스가 배치된 이벤트의 구간(시작·끝 시간)을 몽타주에서 찾는다. 캐싱·검증이 공유한다. */
	bool TryGetNotifyWindow(const class UAnimMontage* Montage, float& OutStartTime, float& OutEndTime) const;

#endif

	/** 구간 스윕을 수행하는 디텍터. */
	UPROPERTY(EditAnywhere, Instanced, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOverdriveCombatHitSweepDetector> HitDetector;

	/** 공격자에게 전송할 GameplayEvent 태그. 기본값은 Combat.Event.Hit 이고, 지워서 비우면 전송 시 그 기본 태그로 대체된다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true", Categories = "Combat.Event"))
	FGameplayTag EventTag;

	/** 컴포넌트 상대 공격 원점. 기즈모로 편집하며, 히트마다 TargetData 에 실려 넉백/파동 중심으로 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	FVector AttackOrigin = FVector::ZeroVector;

	/** ImpactNormal 재계산 스펙. Mode 가 None 이면 히트가 준 노멀을 그대로 둔다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	FOverdriveCombatImpactNormalSpec ImpactNormalSpec;

#if WITH_EDITORONLY_DATA
	/**
	 * 서브스텝 1개의 시간 길이(초). 노티파이 구간을 이 간격으로 나눠 샘플한다. 베이크(캐싱)할 때만 쓰는 에디터 전용 값이다.
	 * 세그먼트 수 = Max(1, CeilToInt(Duration / SubStepTime)), 키프레임 수 = 세그먼트 + 1. 작을수록 궤적을 촘촘히 따라간다.
	 */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true", ClampMin = "0.005", ForceUnits = "s"))
	float SubStepTime = 0.02f;

	/** CacheAttackKeyframes 를 실행하는 버튼 자리. 저장되는 값은 없다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	FOverdriveCombatCacheKeyframesButton CacheKeyframesButton;

	/** 마지막 베이크 시점의 노티파이 구간 시작(몽타주 트랙 시간). 현재 구간과 다르면 캐시가 낡은 것이다. */
	UPROPERTY()
	float CachedStartTime = 0.0f;

	/** 마지막 베이크 시점의 노티파이 구간 끝(몽타주 트랙 시간). 시작과 함께 스테일 판정에 쓴다. */
	UPROPERTY()
	float CachedEndTime = 0.0f;
#endif

	/** 에디터에서 베이크된 컴포넌트 상대 앵커 키프레임들. 런타임은 이 값을 그대로 스윕에 쓴다. */
	UPROPERTY()
	TArray<FOverdriveCombatAttackKeyframe> CachedKeyframes;

	/**
	 * 노티파이 인스턴스는 애님 애셋당 하나뿐이고 여러 메시가 공유하므로, 런타임 상태는 메시별로 보관한다.
	 * 강참조는 GC 를 막으므로 약참조 키를 쓴다.
	 */
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FInstanceRuntimeState> RuntimeStateMap;
};
