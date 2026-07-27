// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditModes/OverdriveCombatDetectorEditMode.h"

class UAnimSequenceBase;
class UOverdriveCombatAnimNotifyState_Attack;
class UOverdriveCombatHitSweepDetector;
struct FOverdriveCombatImpactNormalSpec;

/**
 * 구간 스윕 Attack 노티파이 스테이트 전용 에디트 모드.
 *
 * 셰이프가 앵커 소켓을 따라가므로 부모 프레임 = (앵커 소켓 x 컴포넌트) 월드다.
 * 공용 로직(핸들 전환·트랜잭션·기즈모)은 베이스에 있고, 여기서는 디텍터/노티파이 접근만 위임한다.
 */
class FOverdriveCombatSweepDetectorEditMode : public FOverdriveCombatDetectorEditMode
{
public:
	static const FEditorModeID ModeID;

	/** 편집 대상 디텍터·노티파이·소유 애님 에셋을 지정한다. 모듈의 선택 핸들러가 호출한다. */
	void SetTarget(UOverdriveCombatHitSweepDetector* InDetector, UOverdriveCombatAnimNotifyState_Attack* InNotify, UAnimSequenceBase* InAnimAsset);
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
	/** 앵커 소켓의 컴포넌트 상대 트랜스폼(소켓 없으면 Identity → 컴포넌트 루트). */
	FTransform GetAnchorComponentSpace() const;

	/** 편집 대상 디텍터. 애님 에셋이 소유한 인스턴스이므로 약참조로만 잡는다. */
	TWeakObjectPtr<UOverdriveCombatHitSweepDetector> TargetDetector;

	/** 공격 원점을 소유한 노티파이 스테이트. */
	TWeakObjectPtr<UOverdriveCombatAnimNotifyState_Attack> TargetNotify;
};
