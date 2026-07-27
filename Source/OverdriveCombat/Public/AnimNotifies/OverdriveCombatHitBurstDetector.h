// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/EngineTypes.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "OverdriveCombatHitBurstDetector.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;
struct FGameplayAbilityTargetDataHandle;
struct FHitResult;

/**
 * 애님 노티파이가 사용하는 히트 판정 디텍터.
 *
 * 히트마다 AttackTypeTag 를 실은 FOverdriveCombatTargetData_AttackHit 을 만들어
 * FGameplayAbilityTargetDataHandle 에 채우고, 노티파이는 그 핸들로 GameplayEvent 만 보낸다.
 *
 */
UCLASS(Abstract, BlueprintType, NotBlueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories)
class OVERDRIVECOMBAT_API UOverdriveCombatHitBurstDetector : public UObject
{
	GENERATED_BODY()

public:
	int32 DetectHit(const FOverdriveCombatHitBurstDetectorContext& Context, FGameplayAbilityTargetDataHandle& OutTargetData) const;

	/** 셰이프 배치 트랜스폼(메시 컴포넌트 공간). */
	FTransform GetRelativeTransform() const { return RelativeTransform; }

	/** 셰이프 배치 트랜스폼을 설정한다. 스케일은 판정에 쓰지 않으므로 (1,1,1)로 강제한다. */
	void SetRelativeTransform(const FTransform& InTransform);

#if WITH_EDITOR
	/** 노티파이 표시 이름 / 디버그용. */
	virtual FString GetDetectorDisplayName() const { return TEXT("Detector"); }

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** RelativeTransform 이 private 이므로 에디터 모듈이 쓰는 프로퍼티 이름 통로. */
	static FName GetRelativeTransformPropertyName();

	/** 셰이프 와이어프레임을 그린다. 선택 시 에디트 모드 Render 가 쓴다. 파생 디텍터가 구현한다. */
	virtual void DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const;
#endif

#if ENABLE_DRAW_DEBUG
	/** 물리 없이 판정 셰이프(스윕 볼륨)만 디버그 드로우한다. 에디터 프리뷰 시각화용. 파생 디텍터가 구현한다. */
	virtual void DrawDebugDetection(const FOverdriveCombatHitBurstDetectorContext& Context, int32 OverrideDrawMode = -1) const {}
#endif

protected:

	/**
	 * 셰이프의 월드 트랜스폼. RelativeTransform(메시 컴포넌트 공간)을 컴포넌트 트랜스폼에 합성한다.
	 * FTransform 합성은 Child * Parent 순서다.
	 */
	FTransform CalculateWorldTransform(const USkeletalMeshComponent* MeshComp) const;

	/** 파생 디텍터가 스윕에 쓸 트레이스 채널. */
	ETraceTypeQuery GetTraceChannel() const { return TraceChannel; }

private:
	/**
	 * 히트를 모아 OutHits 에 채운다. 셰이프 배치·스윕 방식은 파생 디텍터가 정한다.
	 * 컴포넌트 중복 제거도 구현 쪽 책임이다.
	 *
	 * 파생에서 반드시 오버라이드한다. UCLASS(Abstract) 로 베이스 인스턴스화를 막으므로
	 * C++ 순수 가상(= 0)은 쓰지 않는다(UObject 리플렉션 생성자가 추상 클래스를 new 하지 못한다).
	 */
	virtual void CollectHits(const FOverdriveCombatHitBurstDetectorContext& Context, TArray<FHitResult>& OutHits) const {}

	/** 히트들을 AttackTypeTag 와 함께 TargetData 로 패킹해 핸들에 추가한다. */
	void PackTargetData(const TArray<FHitResult>& Hits, FGameplayAbilityTargetDataHandle& OutTargetData) const;

	/**
	 * 스켈레탈 메시 컴포넌트 공간 기준 셰이프 배치.
	 * 소켓이 아니라 컴포넌트에 고정되므로 포즈 / 프레임 차이에 흔들리지 않는 결정적 판정을 만든다.
	 * 위치와 회전만 사용하고 스케일은 무시한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (AllowPrivateAccess = "true"))
	FTransform RelativeTransform = FTransform::Identity;

	/** 이 디텍터가 만들어내는 공격의 타입. 히트마다 TargetData 에 실려 수신 어빌리티까지 전달된다. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (AllowPrivateAccess = "true", Categories = "Combat.Attack"))
	FGameplayTag AttackTypeTag;

	/** 스윕에 사용할 트레이스 채널. 무엇을 맞출지는 대상의 콜리전 프리셋에서 이 채널 응답으로 제어한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ETraceTypeQuery> TraceChannel = TraceTypeQuery1;
};
