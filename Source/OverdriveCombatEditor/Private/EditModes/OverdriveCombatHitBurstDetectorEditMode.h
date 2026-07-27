// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditModes/OverdriveCombatDetectorEditMode.h"

class UAnimSequenceBase;
class UOverdriveCombatAnimNotify_Attack;
class UOverdriveCombatHitBurstDetector;
struct FOverdriveCombatImpactNormalSpec;

/**
 * 단발 Attack 노티파이 전용 에디트 모드.
 *
 * 셰이프는 앵커 소켓 없이 메시 컴포넌트 공간에 고정되므로 부모 프레임 = 컴포넌트 트랜스폼이다.
 * 공용 로직(핸들 전환·트랜잭션·기즈모)은 베이스에 있고, 여기서는 디텍터/노티파이 접근만 위임한다.
 */
class FOverdriveCombatHitBurstDetectorEditMode : public FOverdriveCombatDetectorEditMode
{
public:
	static const FEditorModeID ModeID;

	/** 편집 대상 디텍터·노티파이·소유 애님 에셋을 지정한다. 모듈의 선택 핸들러가 호출한다. */
	void SetTarget(UOverdriveCombatHitBurstDetector* InDetector, UOverdriveCombatAnimNotify_Attack* InNotify, UAnimSequenceBase* InAnimAsset);
	void ClearTarget();

protected:
	/** 셰이프 훅 */
	virtual bool HasShapeTarget() const override;
	virtual FTransform GetShapeAnchorToWorld() const override;
	virtual FTransform GetShapeRelativeTransform() const override;
	virtual void SetShapeRelativeTransform(const FTransform& InTransform) override;
	virtual bool ShapeUsesRotation() const override { return true; }
	virtual void RenderShape(FPrimitiveDrawInterface* PDI, const FLinearColor& Color) const override;
	virtual UObject* GetShapeOwnerObject() const override;
	virtual FName GetShapeTransformPropertyName() const override;
	virtual FString GetShapeLabel() const override;

	/** Origin 훅 */
	virtual bool HasOriginTarget() const override;
	virtual FVector GetOriginLocal() const override;
	virtual void SetOriginLocal(const FVector& InLocal) override;
	virtual UObject* GetOriginOwnerObject() const override;
	virtual FName GetOriginPropertyName() const override;

	/** Direction 훅 */
	virtual FOverdriveCombatImpactNormalSpec* GetImpactNormalSpec() const override;
	virtual FName GetImpactNormalSpecPropertyName() const override;

private:
	/** 편집 대상 디텍터. 애님 에셋이 소유한 인스턴스이므로 약참조로만 잡는다. */
	TWeakObjectPtr<UOverdriveCombatHitBurstDetector> TargetDetector;

	/** 공격 원점을 소유한 노티파이. */
	TWeakObjectPtr<UOverdriveCombatAnimNotify_Attack> TargetNotify;
};
