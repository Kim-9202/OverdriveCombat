// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "OverdriveCombatHitSweepDetector.generated.h"

class AActor;
class FPrimitiveDrawInterface;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class UWorld;
struct FCollisionShape;
struct FGameplayAbilityTargetDataHandle;
struct FHitResult;

/** 스윕 디텍터가 지원하는 셰이프 종류. */
UENUM()
enum class EOverdriveCombatSweepShapeType : uint8
{
	Sphere,
	Capsule,
	Box
};

/**
 * 구간(NotifyState) 판정용 스윕 디텍터.
 *
 * 노티파이 스테이트가 앵커 소켓의 컴포넌트 상대 트랜스폼을 구간에 걸쳐 여러 시점 샘플링해 넘기면,
 * 인접 샘플마다 셰이프를 스윕해 스윙 궤적 전체의 히트를 한 번에 모은다. 셰이프는 구 / 캡슐 / 박스 중 하나.
 *
 * 단발 디텍터(UOverdriveCombatHitBurstDetector)와 상속 관계가 없는 독립 디텍터다.
 * 판정 트랜스폼·태그·트레이스 채널과 TargetData 패킹·에디터 통로를 자체적으로 갖는다.
 * 히트 이벤트 전송과 TargetData 타입만 공용 유틸(OverdriveCombatHitEvents / FOverdriveCombatTargetData_AttackHit)을 쓴다.
 */
UCLASS(BlueprintType, NotBlueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories, meta = (DisplayName = "Simple Shape (Sweep)"))
class OVERDRIVECOMBAT_API UOverdriveCombatHitSweepDetector : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 컴포넌트 상대 공간 앵커 샘플 한 쌍(세그먼트)을 스윕한다.
	 * 각 샘플에 RelativeTransform(소켓 로컬 오프셋)을 얹고 ComponentToWorld 로 월드 배치한다.
	 *
	 * AlreadyHitComponents 는 이전 틱에서 이미 확정·전송된 컴포넌트의 읽기 전용 스킵셋이다(크로스틱 중복 방지).
	 * 스킵셋에 없는 히트는 InOutTickBestHits(이번 틱 최적맵)에 누적하되, 같은 컴포넌트가 여러 세그먼트에
	 * 걸치면 가장 빠른 히트가 아니라 WorldOrigin 최근접 히트만 남긴다. 패킹·확정은 CommitTickHits 가 한다.
	 */
	void DetectHitForSegment(const USkeletalMeshComponent* MeshComp, const AActor* OwnerActor, const FTransform& StartSampleCompSpace, const FTransform& EndSampleCompSpace, const FTransform& ComponentToWorld, const FVector& WorldOrigin, const TSet<TWeakObjectPtr<UPrimitiveComponent>>& AlreadyHitComponents, TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& InOutTickBestHits) const;

	/**
	 * 한 틱 동안 모은 최적 히트맵을 TargetData 로 패킹하고, 그 컴포넌트들을 AlreadyHitComponents 에 확정 반영한다.
	 * 확정된 컴포넌트는 다음 틱의 DetectHitForSegment 에서 스킵된다.
	 */
	void CommitTickHits(const TMap<TWeakObjectPtr<UPrimitiveComponent>, FHitResult>& TickBestHits, TSet<TWeakObjectPtr<UPrimitiveComponent>>& InOutAlreadyHitComponents, FGameplayAbilityTargetDataHandle& OutTargetData) const;

	/** 셰이프가 따라붙는 앵커 소켓. 비우면(NAME_None) 컴포넌트 루트. */
	FName GetSocketName() const { return SocketName; }

	/** 셰이프 배치 오프셋(앵커 소켓 로컬 공간). */
	FTransform GetRelativeTransform() const { return RelativeTransform; }

	/** 셰이프 배치 오프셋을 설정한다. 스케일은 판정에 쓰지 않으므로 (1,1,1)로 강제한다. */
	void SetRelativeTransform(const FTransform& InTransform);

#if WITH_EDITOR
	/** 노티파이 표시 이름 / 디버그용. */
	FString GetDetectorDisplayName() const { return TEXT("Simple Shape (Sweep)"); }

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

	/** RelativeTransform 이 private 이므로 에디터 모듈이 쓰는 프로퍼티 이름 통로. */
	static FName GetRelativeTransformPropertyName();

	/** 프리뷰 포즈의 소켓 위치에 정적 셰이프 하나를 그린다. 베이크된 궤적이 없을 때의 저작 표시용 폴백이다. */
	void DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const;

	/**
	 * 베이크된 앵커 샘플 전체를 궤적으로 그린다. 인접 샘플 한 쌍(세그먼트)마다 스윕 볼륨 실루엣을 그리고,
	 * 샘플 중심을 잇는 폴리라인을 그 위에 얹는다. 실루엣 모양은 런타임 디버그 드로우와 같다.
	 * 노티파이가 선택돼 에디트 모드가 살아 있는 동안에만 호출된다(런타임 디버그 드로우와 무관).
	 *
	 * 인접 세그먼트의 끝·시작 셰이프는 같은 위치에 겹쳐 그려진다(같은 색이라 시각적으로 무해).
	 */
	void DrawEditorSweepPath(FPrimitiveDrawInterface* PDI, const TArray<FTransform>& SamplesCompSpace, const FTransform& ComponentToWorld, const FLinearColor& ShapeColor, const FLinearColor& PathColor) const;
#endif

#if ENABLE_DRAW_DEBUG
	/** 인접 샘플 한 쌍(세그먼트)의 스윕 볼륨을 그린다. 판정이 진행되는 시점에 세그먼트 단위로 호출한다. */
	void DrawDebugSweepSegment(const UWorld* World, const FTransform& StartSampleCompSpace, const FTransform& EndSampleCompSpace, const FTransform& ComponentToWorld) const;
#endif

private:
	/** 현재 셰이프 종류·치수로 콜리전 셰이프를 만든다. */
	FCollisionShape MakeCollisionShape() const;

	/** 인접 샘플의 월드 배치. Child * Parent: RelativeTransform(소켓 로컬) * Sample(컴포넌트 공간) * ComponentToWorld. */
	FTransform ComposeWorldPlacement(const FTransform& SampleCompSpace, const FTransform& ComponentToWorld) const;

	/** 셰이프 배치 오프셋(앵커 소켓 로컬). 위치와 회전만 사용하고 스케일은 무시한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shape", meta = (AllowPrivateAccess = "true"))
	FTransform RelativeTransform = FTransform::Identity;

	/** 셰이프 종류. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true"))
	EOverdriveCombatSweepShapeType ShapeType = EOverdriveCombatSweepShapeType::Sphere;

	/** 구 / 캡슐 반지름. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true", EditCondition = "ShapeType != EOverdriveCombatSweepShapeType::Box", EditConditionHides, ClampMin = "0.0"))
	float Radius = 20.0f;

	/** 캡슐 반높이. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true", EditCondition = "ShapeType == EOverdriveCombatSweepShapeType::Capsule", EditConditionHides, ClampMin = "0.0"))
	float HalfHeight = 40.0f;

	/** 박스 절반 크기. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true", EditCondition = "ShapeType == EOverdriveCombatSweepShapeType::Box", EditConditionHides))
	FVector BoxExtent = FVector(20.0f);

	/** 셰이프가 따라붙는 소켓 / 본. 비우면 컴포넌트 루트를 앵커로 쓴다. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true", AnimNotifyBoneName = "true"))
	FName SocketName;

	/** 이 디텍터가 만들어내는 공격의 타입. 히트마다 TargetData 에 실려 수신 어빌리티까지 전달된다. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (AllowPrivateAccess = "true", Categories = "Combat.Attack"))
	FGameplayTag AttackTypeTag;

	/** 스윕에 사용할 트레이스 채널. 무엇을 맞출지는 대상의 콜리전 프리셋에서 이 채널 응답으로 제어한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ETraceTypeQuery> TraceChannel = TraceTypeQuery1;
};
