// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "OverdriveCombatAnimNotifyState_Attack.generated.h"

class UOverdriveCombatHitSweepDetector;
class UPrimitiveComponent;

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

	/** 노티파이 윈도우 로컬 시간(초). 런타임은 Elapsed 가 이 값을 지날 때 스윕한다. */
	UPROPERTY()
	float Time = 0.0f;
};

/**
 * 구간 스윕 공격 판정 노티파이 스테이트.
 *
 * 앵커 소켓 궤적은 런타임이 아니라 에디터에서 미리 캐싱한다. 디테일 패널의 Cache 버튼(CacheAttackKeyframes)을 누르면
 * 노티파이 구간을 SubStepTime(초) 단위로 나눠, 각 시점의 소켓 컴포넌트 상대 트랜스폼과 윈도우 로컬 시간을 몽타주
 * 슬롯에서 평가해 CachedKeyframes 에 직렬화한다. 런타임은 샘플링 없이 이 캐시를 그대로 사용한다.
 *
 * 런타임에는 경과 시간(Elapsed)이 각 키프레임의 Time 을 지날 때마다 인접 키프레임을 현재 컴포넌트 트랜스폼으로 스윕하고,
 * 그 틱에 나온 신규 히트를 모아 곧바로 GameplayEvent 로 전송한다(틱당 1회). 이전 스윕에서 이미 닿은
 * 컴포넌트는 노티파이 구간 전체에 걸쳐 무시한다. 서버 권위에서만 이벤트를 보낸다.
 *
 * 컴포넌트 상대 공간으로 캐싱하므로 캐릭터 로코모션이 스윕 볼륨을 부풀리지 않는다.
 * 애님 에디터 프리뷰에서는 물리 판정 없이 캐싱 궤적을 디버그 드로우(od.Combat.DrawHitDetection)만 한다.
 *
 * 노티파이 인스턴스는 애님 애셋당 하나뿐이고 여러 메시가 공유하므로, 런타임 상태는 메시별로 보관한다.
 */
UCLASS(EditInlineNew, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "Overdrive Combat Attack (Sweep)"))
class OVERDRIVECOMBAT_API UOverdriveCombatAnimNotifyState_Attack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
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
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	/** 메시 인스턴스별 런타임 스윕 상태. 샘플링은 하지 않고 진행 세그먼트와 누적 중복만 추적한다. */
	struct FInstanceRuntimeState
	{
		/** NotifyBegin 이후 누적 경과 시간. 키프레임 Time 과 비교해 스윕 시점을 판단한다. */
		float Elapsed = 0.0f;

		/** 다음에 스윕할 세그먼트 인덱스(0 .. N-1). */
		int32 NextSegment = 0;

		/** 이미 닿은 컴포넌트. 노티파이 구간 전체에 걸쳐 중복 히트를 막는다. */
		TSet<TWeakObjectPtr<UPrimitiveComponent>> AlreadyHitComponents;
	};

#if WITH_EDITOR
	/** 캐싱된 포즈에서 앵커(소켓/본/루트)의 컴포넌트 상대 트랜스폼을 해석한다. 런타임 GetSocketTransform 와 동일 의미. */
	FTransform ResolveAnchorComponentSpace(const struct FAnimPose& Pose, FName AnchorName, const class USkeletalMesh* Mesh) const;

	/** 이 노티파이 인스턴스가 배치된 이벤트의 구간(시작·길이)을 몽타주에서 찾는다. 캐싱·검증이 공유한다. */
	bool TryGetNotifyWindow(const class UAnimMontage* Montage, float& OutStartTime, float& OutDuration) const;
#endif

	/** 구간 스윕을 수행하는 디텍터. */
	UPROPERTY(EditAnywhere, Instanced, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOverdriveCombatHitSweepDetector> HitDetector;

	/** 공격자에게 전송할 GameplayEvent 태그. 비워 두면 Combat.Event.Hit 을 쓴다. */
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

	/** 마지막 베이크 시점의 노티파이 윈도우 길이(초). 현재 길이와 다르면 캐시가 스테일임을 알린다. */
	UPROPERTY()
	float CachedTotalDuration = 0.0f;
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
