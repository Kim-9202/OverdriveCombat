// Fill out your copyright notice in the Description page of Project Settings.


#include "EditModes/OverdriveCombatSweepDetectorEditMode.h"

#include "AnimNotifies/OverdriveCombatAnimNotifyState_Attack.h"
#include "AnimNotifies/OverdriveCombatHitSweepDetector.h"
#include "Animation/DebugSkelMeshComponent.h"

const FEditorModeID FOverdriveCombatSweepDetectorEditMode::ModeID(TEXT("OverdriveCombat.SweepDetectorEditMode"));

namespace
{
	/** 베이크 궤적의 셰이프 와이어 색. 경로선에 묻히도록 어둡게 깔아 배경처럼 보이게 한다. */
	const FLinearColor SweepPathShapeColor(0.45f, 0.22f, 0.0f);
}

void FOverdriveCombatSweepDetectorEditMode::SetTarget(UOverdriveCombatHitSweepDetector* InDetector, UOverdriveCombatAnimNotifyState_Attack* InNotify, UAnimSequenceBase* InAnimAsset)
{
	TargetDetector = InDetector;
	TargetNotify = InNotify;
	TargetAnimAsset = InAnimAsset;

	// 편집 대상이 바뀌면 이전 선택은 무효다. Direction 이 남으면 새 대상에 방향이
	// 없을 때 위젯이 사라져 갇히므로 항상 Shape 로 되돌린다.
	SelectedHandle = EOverdriveCombatHandle::Shape;
}

void FOverdriveCombatSweepDetectorEditMode::ClearTarget()
{
	TargetDetector.Reset();
	TargetNotify.Reset();
	TargetAnimAsset.Reset();
	SelectedHandle = EOverdriveCombatHandle::Shape;
}

FTransform FOverdriveCombatSweepDetectorEditMode::GetAnchorComponentSpace() const
{
	const UOverdriveCombatHitSweepDetector* Detector = TargetDetector.Get();
	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	if (Detector == nullptr || PreviewMeshComponent == nullptr)
	{
		return FTransform::Identity;
	}

	// 앵커 소켓이 없으면(NAME_None) RTS_Component 는 Identity 를 돌려주므로 컴포넌트 루트가 기준이 된다.
	return PreviewMeshComponent->GetSocketTransform(Detector->GetSocketName(), RTS_Component);
}

bool FOverdriveCombatSweepDetectorEditMode::HasShapeTarget() const
{
	return TargetDetector.IsValid();
}

FTransform FOverdriveCombatSweepDetectorEditMode::GetShapeAnchorToWorld() const
{
	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	if (PreviewMeshComponent == nullptr)
	{
		return FTransform::Identity;
	}

	// 부모 프레임 = 앵커 소켓(컴포넌트 공간) x 컴포넌트(월드).
	return GetAnchorComponentSpace() * PreviewMeshComponent->GetComponentTransform();
}

FTransform FOverdriveCombatSweepDetectorEditMode::GetShapeRelativeTransform() const
{
	const UOverdriveCombatHitSweepDetector* Detector = TargetDetector.Get();
	return (Detector != nullptr) ? Detector->GetRelativeTransform() : FTransform::Identity;
}

void FOverdriveCombatSweepDetectorEditMode::SetShapeRelativeTransform(const FTransform& InTransform)
{
	if (UOverdriveCombatHitSweepDetector* Detector = TargetDetector.Get())
	{
		Detector->SetRelativeTransform(InTransform);
	}
}

void FOverdriveCombatSweepDetectorEditMode::RenderShape(FPrimitiveDrawInterface* PDI, const FLinearColor& Color) const
{
	const UOverdriveCombatHitSweepDetector* Detector = TargetDetector.Get();
	if (Detector == nullptr)
	{
		return;
	}

	const UOverdriveCombatAnimNotifyState_Attack* Notify = TargetNotify.Get();
	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	const int32 KeyframeCount = (Notify != nullptr) ? Notify->GetCachedKeyframes().Num() : 0;

	// 베이크된 궤적이 있으면 구간 전체를 그린다. 현재 포즈의 셰이프는 이 궤적에 이미 포함되므로 따로 그리지 않는다.
	if (KeyframeCount >= 2 && PreviewMeshComponent != nullptr)
	{
		// 디텍터는 트랜스폼 배열을 받으므로 키프레임에서 트랜스폼만 뽑아 넘긴다.
		TArray<FTransform> SamplesCompSpace;
		SamplesCompSpace.Reserve(KeyframeCount);
		for (const FOverdriveCombatAttackKeyframe& Keyframe : Notify->GetCachedKeyframes())
		{
			SamplesCompSpace.Add(Keyframe.Transform);
		}

		// 경로선에는 선택 강조색을 그대로 쓴다(셰이프 핸들 선택 여부가 궤적에도 드러난다).
		Detector->DrawEditorSweepPath(PDI, SamplesCompSpace, PreviewMeshComponent->GetComponentTransform(), SweepPathShapeColor, Color);

		return;
	}

	// 베이크 전에는 궤적이 없으므로 현재 포즈 위치에 셰이프 하나만 그려 치수·오프셋 편집 피드백을 남긴다.
	Detector->DrawEditorShapes(PDI, PreviewMeshComponent, Color);
}

UObject* FOverdriveCombatSweepDetectorEditMode::GetShapeOwnerObject() const
{
	return TargetDetector.Get();
}

FName FOverdriveCombatSweepDetectorEditMode::GetShapeTransformPropertyName() const
{
	return UOverdriveCombatHitSweepDetector::GetRelativeTransformPropertyName();
}

FString FOverdriveCombatSweepDetectorEditMode::GetShapeLabel() const
{
	const UOverdriveCombatHitSweepDetector* Detector = TargetDetector.Get();
	return (Detector != nullptr) ? Detector->GetDetectorDisplayName() : FString();
}

bool FOverdriveCombatSweepDetectorEditMode::HasOriginTarget() const
{
	return TargetNotify.IsValid();
}

FVector FOverdriveCombatSweepDetectorEditMode::GetOriginLocal() const
{
	const UOverdriveCombatAnimNotifyState_Attack* Notify = TargetNotify.Get();
	return (Notify != nullptr) ? Notify->GetAttackOrigin() : FVector::ZeroVector;
}

void FOverdriveCombatSweepDetectorEditMode::SetOriginLocal(const FVector& InLocal)
{
	if (UOverdriveCombatAnimNotifyState_Attack* Notify = TargetNotify.Get())
	{
		Notify->SetAttackOrigin(InLocal);
	}
}

UObject* FOverdriveCombatSweepDetectorEditMode::GetOriginOwnerObject() const
{
	return TargetNotify.Get();
}

FName FOverdriveCombatSweepDetectorEditMode::GetOriginPropertyName() const
{
	return UOverdriveCombatAnimNotifyState_Attack::GetAttackOriginPropertyName();
}

FOverdriveCombatImpactNormalSpec* FOverdriveCombatSweepDetectorEditMode::GetImpactNormalSpec() const
{
	UOverdriveCombatAnimNotifyState_Attack* Notify = TargetNotify.Get();
	return (Notify != nullptr) ? &Notify->GetImpactNormalSpecForEdit() : nullptr;
}

FName FOverdriveCombatSweepDetectorEditMode::GetImpactNormalSpecPropertyName() const
{
	return UOverdriveCombatAnimNotifyState_Attack::GetImpactNormalSpecPropertyName();
}
