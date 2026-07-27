// Fill out your copyright notice in the Description page of Project Settings.


#include "EditModes/OverdriveCombatHitBurstDetectorEditMode.h"

#include "AnimNotifies/OverdriveCombatAnimNotify_Attack.h"
#include "AnimNotifies/OverdriveCombatHitBurstDetector.h"
#include "Animation/DebugSkelMeshComponent.h"

const FEditorModeID FOverdriveCombatHitBurstDetectorEditMode::ModeID(TEXT("OverdriveCombat.HitDetectorEditMode"));

void FOverdriveCombatHitBurstDetectorEditMode::SetTarget(UOverdriveCombatHitBurstDetector* InDetector, UOverdriveCombatAnimNotify_Attack* InNotify, UAnimSequenceBase* InAnimAsset)
{
	TargetDetector = InDetector;
	TargetNotify = InNotify;
	TargetAnimAsset = InAnimAsset;

	// 편집 대상이 바뀌면 이전 선택은 무효다. Direction 이 남으면 새 대상에 방향이
	// 없을 때 위젯이 사라져 갇히므로 항상 Shape 로 되돌린다.
	SelectedHandle = EOverdriveCombatHandle::Shape;
}

void FOverdriveCombatHitBurstDetectorEditMode::ClearTarget()
{
	TargetDetector.Reset();
	TargetNotify.Reset();
	TargetAnimAsset.Reset();
	SelectedHandle = EOverdriveCombatHandle::Shape;
}

bool FOverdriveCombatHitBurstDetectorEditMode::HasShapeTarget() const
{
	return TargetDetector.IsValid();
}

FTransform FOverdriveCombatHitBurstDetectorEditMode::GetShapeAnchorToWorld() const
{
	// 앵커 소켓 없이 컴포넌트에 고정된다: 부모 프레임 = 컴포넌트 트랜스폼.
	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	return (PreviewMeshComponent != nullptr) ? PreviewMeshComponent->GetComponentTransform() : FTransform::Identity;
}

FTransform FOverdriveCombatHitBurstDetectorEditMode::GetShapeRelativeTransform() const
{
	const UOverdriveCombatHitBurstDetector* Detector = TargetDetector.Get();
	return (Detector != nullptr) ? Detector->GetRelativeTransform() : FTransform::Identity;
}

void FOverdriveCombatHitBurstDetectorEditMode::SetShapeRelativeTransform(const FTransform& InTransform)
{
	if (UOverdriveCombatHitBurstDetector* Detector = TargetDetector.Get())
	{
		Detector->SetRelativeTransform(InTransform);
	}
}

void FOverdriveCombatHitBurstDetectorEditMode::RenderShape(FPrimitiveDrawInterface* PDI, const FLinearColor& Color) const
{
	if (const UOverdriveCombatHitBurstDetector* Detector = TargetDetector.Get())
	{
		Detector->DrawEditorShapes(PDI, GetPreviewMeshComponent(), Color);
	}
}

UObject* FOverdriveCombatHitBurstDetectorEditMode::GetShapeOwnerObject() const
{
	return TargetDetector.Get();
}

FName FOverdriveCombatHitBurstDetectorEditMode::GetShapeTransformPropertyName() const
{
	return UOverdriveCombatHitBurstDetector::GetRelativeTransformPropertyName();
}

FString FOverdriveCombatHitBurstDetectorEditMode::GetShapeLabel() const
{
	const UOverdriveCombatHitBurstDetector* Detector = TargetDetector.Get();
	return (Detector != nullptr) ? Detector->GetDetectorDisplayName() : FString();
}

bool FOverdriveCombatHitBurstDetectorEditMode::HasOriginTarget() const
{
	return TargetNotify.IsValid();
}

FVector FOverdriveCombatHitBurstDetectorEditMode::GetOriginLocal() const
{
	const UOverdriveCombatAnimNotify_Attack* Notify = TargetNotify.Get();
	return (Notify != nullptr) ? Notify->GetAttackOrigin() : FVector::ZeroVector;
}

void FOverdriveCombatHitBurstDetectorEditMode::SetOriginLocal(const FVector& InLocal)
{
	if (UOverdriveCombatAnimNotify_Attack* Notify = TargetNotify.Get())
	{
		Notify->SetAttackOrigin(InLocal);
	}
}

UObject* FOverdriveCombatHitBurstDetectorEditMode::GetOriginOwnerObject() const
{
	return TargetNotify.Get();
}

FName FOverdriveCombatHitBurstDetectorEditMode::GetOriginPropertyName() const
{
	return UOverdriveCombatAnimNotify_Attack::GetAttackOriginPropertyName();
}

FOverdriveCombatImpactNormalSpec* FOverdriveCombatHitBurstDetectorEditMode::GetImpactNormalSpec() const
{
	UOverdriveCombatAnimNotify_Attack* Notify = TargetNotify.Get();
	return (Notify != nullptr) ? &Notify->GetImpactNormalSpecForEdit() : nullptr;
}

FName FOverdriveCombatHitBurstDetectorEditMode::GetImpactNormalSpecPropertyName() const
{
	return UOverdriveCombatAnimNotify_Attack::GetImpactNormalSpecPropertyName();
}
