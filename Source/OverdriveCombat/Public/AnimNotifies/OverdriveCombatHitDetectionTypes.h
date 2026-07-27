// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "CollisionShape.h"
#include "GameplayTagContainer.h"
#include "OverdriveCombatHitDetectionTypes.generated.h"

class AActor;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class UWorld;
struct FGameplayAbilityTargetDataHandle;
struct FHitResult;

/**
 * 단발 판정 디텍터에 넘기는 스택 전용 컨텍스트.
 */
struct FOverdriveCombatHitBurstDetectorContext
{
	/** 노티파이가 붙어 있는 스켈레탈 메시. */
	const USkeletalMeshComponent* MeshComp = nullptr;

	/**
	 * 메시의 소유 액터. 트레이스 제외 대상이자 히트 필터의 기준이므로 반드시 유효해야 한다.
	 */
	const AActor* OwnerActor = nullptr;

	/** 컴포넌트 상대 Origin 을 월드로 변환한 값. 같은 컴포넌트 중복 시 최근접 히트 선택에 쓴다. */
	FVector WorldOrigin = FVector::ZeroVector;
};

/**
 * ImpactNormal 재계산에 필요한 스택 전용 컨텍스트.
 *
 * USTRUCT 이 아니라 순수 C++ 구조체다(리플렉션 노출 경로가 없다).
 */
struct FOverdriveCombatImpactContext
{
	/** 컴포넌트 상대 Origin 을 월드로 변환한 값. */
	FVector WorldOrigin = FVector::ZeroVector;

	/** 지정 방향(컴포넌트 공간)을 월드로 회전하는 데 쓰는 메시 컴포넌트 트랜스폼. */
	FTransform ComponentToWorld = FTransform::Identity;
};

/**
 * 히트 ImpactNormal 재계산 규칙.
 *
 * 커스텀 확장은 지원하지 않는다 — 여기 없는 규칙이 필요하면 이벤트를 받은 어빌리티가
 * TargetData 의 HitResult/GetOrigin() 으로 직접 계산한다.
 */
UENUM()
enum class EOverdriveCombatImpactNormalMode : uint8
{
	/** 히트가 준 노멀을 그대로 둔다. */
	None			UMETA(DisplayName = "Keep Hit Normal"),

	/** Origin → 히트 지점(바깥으로 뻗는 방향). */
	FromOrigin		UMETA(DisplayName = "From Origin (Origin -> Impact)"),

	/** 히트 지점 → Origin(중심으로 빨려드는 방향). */
	TowardOrigin	UMETA(DisplayName = "Toward Origin (Impact -> Origin)"),

	/** 컴포넌트 공간 기준 지정 방향. */
	FixedDirection	UMETA(DisplayName = "Fixed Direction"),
};

/**
 * ImpactNormal 재계산 스펙. Attack 노티파이 2종(단발/구간)이 공유한다.
 * Direction 은 에디터 뷰포트 기즈모(FOverdriveCombatDetectorEditMode)로도 편집된다.
 */
USTRUCT()
struct OVERDRIVECOMBAT_API FOverdriveCombatImpactNormalSpec
{
	GENERATED_BODY()

	/** 재계산 규칙. None 이면 히트가 준 노멀을 그대로 둔다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat")
	EOverdriveCombatImpactNormalMode Mode = EOverdriveCombatImpactNormalMode::None;

	/** 컴포넌트 공간 기준 고정 방향. 컴포넌트 트랜스폼으로 회전만 적용해 월드 노멀로 쓴다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (EditCondition = "Mode == EOverdriveCombatImpactNormalMode::FixedDirection", EditConditionHides))
	FVector Direction = FVector(1.0f, 0.0f, 0.0f);
};

namespace OverdriveCombatHitEvents
{
	/**
	 * 디텍터가 채운 TargetData 를 공격자에게 GameplayEvent 로 보낸다.
	 *
	 * UAnimNotify 와 UAnimNotifyState 는 공통 베이스가 없으므로, 나중에 구간 노티파이가 생겨도
	 * 이벤트 전송을 그대로 공유할 수 있도록 자유 함수로 둔다.
	 *
	 * Payload.Target 과 EffectContext 의 HitResult 는 TargetData 첫 항목에서 얻는다.
	 * TargetData 가 비어 있으면 아무것도 하지 않는다. EventTag 이 비어 있으면 Combat.Event.Hit 을 쓴다.
	 */
	OVERDRIVECOMBAT_API void SendHitEvent(AActor* InstigatorActor, const FGameplayAbilityTargetDataHandle& TargetData, FGameplayTag EventTag);

	/**
	 * 핸들의 각 FOverdriveCombatTargetData_AttackHit 에 공격 원점(Context.WorldOrigin)을 심고,
	 * Spec.Mode 에 따라 ImpactNormal(과 Normal)을 재계산한다. 전송(SendHitEvent) 직전에 부른다.
	 *
	 * 우리 타입이 아닌 TargetData 는 건너뛴다. Mode 가 None 이면 Origin 만 채운다.
	 */
	OVERDRIVECOMBAT_API void ApplyImpactPostProcess(FGameplayAbilityTargetDataHandle& TargetData, const FOverdriveCombatImpactContext& Context, const FOverdriveCombatImpactNormalSpec& Spec);
}

namespace OverdriveCombatHitDetection
{
	/**
	 * 컴포넌트별 최적 히트 맵에 Hit 를 반영한다.
	 * 같은 컴포넌트가 이미 있으면 ImpactPoint 가 WorldOrigin 에 더 가까운 히트만 남긴다.
	 * (같은 컴포넌트 중복 시 "가장 빠른" 히트가 아니라 원점 최근접 히트를 채택하기 위한 정책.)
	 */
	OVERDRIVECOMBAT_API void ConsiderClosestToOrigin(TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& BestByComponent, const FHitResult& Hit, const FVector& WorldOrigin);
}

/**
 * 히트 판정 디버그 드로우.
 *
 * 판정 로직은 디텍터가 갖고 있고, 여기에는 그리기만 둔다.
 * 콘솔 변수는 이름당 한 번만 등록할 수 있으므로 CVar 정의도 이 모듈 한 곳(cpp)에만 존재한다.
 */
namespace OverdriveCombatDebug
{
#if ENABLE_DRAW_DEBUG
	OVERDRIVECOMBAT_API void DrawShapeSweep(const UWorld* World, const FCollisionShape& Shape, const FVector& Start, const FVector& End, const FQuat& Rotation, const TArray<FHitResult>& Hits, int32 OverrideDrawMode = -1);
#endif
}
