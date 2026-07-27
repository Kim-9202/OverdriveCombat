// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifies/OverdriveCombatHitBurstDetector.h"
#include "OverdriveCombatHitBurstDetector_SimpleShape.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;
struct FCollisionShape;
struct FHitResult;

/** SimpleShape 디텍터가 지원하는 기본 셰이프 종류. */
UENUM()
enum class EOverdriveCombatShapeType : uint8
{
	Sphere,
	Capsule,
	Box
};

/**
 * SimpleShape 디텍터의 셰이프 하나 배치.
 *
 * 위치 / 회전은 디텍터 로컬 공간(= 베이스 RelativeTransform 상대)이며 Start -> End 로 스윕한다.
 * 종류에 따라 사용하는 치수 필드가 달라진다(EditCondition 으로 노출 제어).
 */
USTRUCT(BlueprintType)
struct FOverdriveCombatSimpleShapeEntry
{
	GENERATED_BODY()

	/** 셰이프 종류. */
	UPROPERTY(EditAnywhere, Category = "Shape")
	EOverdriveCombatShapeType ShapeType = EOverdriveCombatShapeType::Sphere;

	/** 구 / 캡슐 반지름. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (EditCondition = "ShapeType != EOverdriveCombatShapeType::Box", EditConditionHides, ClampMin = "0.0"))
	float Radius = 20.0f;

	/** 캡슐 반높이. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (EditCondition = "ShapeType == EOverdriveCombatShapeType::Capsule", EditConditionHides, ClampMin = "0.0"))
	float HalfHeight = 40.0f;

	/** 박스 절반 크기. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (EditCondition = "ShapeType == EOverdriveCombatShapeType::Box", EditConditionHides))
	FVector BoxExtent = FVector(20.0f);

	/** 스윕 시작 위치(디텍터 로컬). */
	UPROPERTY(EditAnywhere, Category = "Shape")
	FVector StartLocation = FVector::ZeroVector;

	/** 스윕 끝 위치(디텍터 로컬). Start 와 같으면 제자리 판정이 된다. */
	UPROPERTY(EditAnywhere, Category = "Shape")
	FVector EndLocation = FVector::ZeroVector;

	/** 셰이프 회전(디텍터 로컬). */
	UPROPERTY(EditAnywhere, Category = "Shape")
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * 기본 셰이프(구 / 캡슐 / 박스) 배열을 각각 Start -> End 로 스윕해 히트를 모으는 디텍터.
 * 히트를 모아 베이스가 AttackTypeTag 를 붙여 TargetData 로 패킹한다.
 */
UCLASS(BlueprintType, NotBlueprintable, EditInlineNew, CollapseCategories, meta = (DisplayName = "Simple Shape"))
class OVERDRIVECOMBAT_API UOverdriveCombatHitBurstDetector_SimpleShape : public UOverdriveCombatHitBurstDetector
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	virtual FString GetDetectorDisplayName() const override { return TEXT("Simple Shape"); }

	virtual void DrawEditorShapes(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FLinearColor& Color) const override;
#endif

#if ENABLE_DRAW_DEBUG
public:
	virtual void DrawDebugDetection(const FOverdriveCombatHitBurstDetectorContext& Context, int32 OverrideDrawMode) const override;
#endif

private:
	/** 셰이프마다 Start -> End 스윕을 돌려 히트를 모은다. 컴포넌트 중복은 이 호출 안에서 제거한다. */
	virtual void CollectHits(const FOverdriveCombatHitBurstDetectorContext& Context, TArray<FHitResult>& OutHits) const override;

	/** 항목 치수로 콜리전 셰이프를 만든다. 스윕과 에디터 드로우가 함께 쓴다. */
	FCollisionShape MakeCollisionShape(const FOverdriveCombatSimpleShapeEntry& Entry) const;

	/** 배치할 셰이프들. */
	UPROPERTY(EditAnywhere, Category = "Shape", meta = (AllowPrivateAccess = "true"))
	TArray<FOverdriveCombatSimpleShapeEntry> Shapes;
};
