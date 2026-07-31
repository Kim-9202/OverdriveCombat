// Fill out your copyright notice in the Description page of Project Settings.


#include "EditModes/OverdriveCombatDetectorEditMode.h"

#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AssetEditorModeManager.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "IPersonaPreviewScene.h"
#include "PrimitiveDrawingUtils.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "OverdriveCombatDetectorEditMode"

namespace
{
	/** 클릭 픽업용 마커 반지름(월드 cm). */
	constexpr float HandleMarkerRadius = 4.0f;

	/** 방향 화살표 길이(월드 cm). Origin 마커(반지름 4)보다 충분히 길어야 집기 쉽다. */
	constexpr float DirectionArrowLength = 40.0f;

	/**
	 * 화살표 시작점을 Origin 에서 띄우는 거리(월드 cm).
	 * 화살표 밑동이 Origin 마커(구 반지름 4 + 십자선 8)를 덮으면 Direction 히트 프록시가
	 * Origin 프록시를 가려 Origin 을 클릭할 수 없다. 십자선 끝의 2배로 띄운다.
	 */
	constexpr float DirectionArrowStartGap = 16.0f;

	/** 화살촉 크기. */
	constexpr float DirectionArrowHeadSize = 6.0f;

	/** 화살표 선 굵기. 얇으면 클릭 픽업이 어렵다. */
	constexpr float DirectionArrowThickness = 2.0f;

	/** 마커 중심에서 라벨을 띄울 화면 오프셋(px). */
	constexpr float LabelScreenOffsetX = 8.0f;
	constexpr float LabelScreenOffsetY = -8.0f;

	/** Origin 핸들 라벨. 셰이프 라벨은 파생이 디텍터 이름으로 준다. */
	const TCHAR* OriginLabel = TEXT("Attack Origin");

	/** Direction 핸들 라벨. */
	const TCHAR* DirectionLabel = TEXT("Impact Normal");

	const FLinearColor ShapeColor(1.0f, 0.5f, 0.0f);
	const FLinearColor ShapeSelectedColor(1.0f, 0.8f, 0.2f);
	const FLinearColor OriginColor(0.0f, 0.7f, 1.0f);
	const FLinearColor OriginSelectedColor(0.3f, 0.9f, 1.0f);
	const FLinearColor DirectionColor(0.2f, 1.0f, 0.4f);
	const FLinearColor DirectionSelectedColor(0.6f, 1.0f, 0.7f);
}

IPersonaPreviewScene& FOverdriveCombatDetectorEditMode::GetAnimPreviewScene() const
{
	// Persona 에디트 모드의 프리뷰 씬 접근 관례(엔진 FSkeletonSelectionEditMode 와 동일).
	return *static_cast<IPersonaPreviewScene*>(static_cast<FAssetEditorModeManager*>(Owner)->GetPreviewScene());
}

UDebugSkelMeshComponent* FOverdriveCombatDetectorEditMode::GetPreviewMeshComponent() const
{
	return GetAnimPreviewScene().GetPreviewMeshComponent();
}

FTransform FOverdriveCombatDetectorEditMode::GetShapeWorldTransform() const
{
	// FTransform 합성은 Child * Parent 순서: 오프셋 -> 부모 프레임(앵커/컴포넌트).
	return GetShapeRelativeTransform() * GetShapeAnchorToWorld();
}

FOverdriveCombatImpactContext FOverdriveCombatDetectorEditMode::MakeImpactContext() const
{
	FOverdriveCombatImpactContext Context;

	if (const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent())
	{
		Context.ComponentToWorld = PreviewMeshComponent->GetComponentTransform();

		if (HasOriginTarget())
		{
			Context.WorldOrigin = Context.ComponentToWorld.TransformPosition(GetOriginLocal());
		}
	}

	return Context;
}

bool FOverdriveCombatDetectorEditMode::HasDirectionTarget() const
{
	// 화살표 기점을 Origin 에서 계산하므로 Origin 대상도 있어야 한다.
	if (GetPreviewMeshComponent() == nullptr || !HasOriginTarget())
	{
		return false;
	}

	return GetFixedDirectionSpec() != nullptr;
}

FOverdriveCombatImpactNormalSpec* FOverdriveCombatDetectorEditMode::GetFixedDirectionSpec() const
{
	// 방향 편집은 FixedDirection 모드 전용이다. 다른 모드는 편집할 방향이 없다.
	FOverdriveCombatImpactNormalSpec* Spec = GetImpactNormalSpec();
	if (Spec == nullptr || Spec->Mode != EOverdriveCombatImpactNormalMode::FixedDirection)
	{
		return nullptr;
	}

	return Spec;
}

FVector FOverdriveCombatDetectorEditMode::GetDirectionWorld(const FOverdriveCombatImpactContext& Context) const
{
	const FOverdriveCombatImpactNormalSpec* Spec = GetFixedDirectionSpec();
	if (Spec == nullptr)
	{
		return FVector::ForwardVector;
	}

	// 컴포넌트 공간 방향을 월드로 회전(스케일 무시). 런타임 후처리(ComputeImpactNormal)와 같은 변환식이다.
	return Context.ComponentToWorld.TransformVectorNoScale(Spec->Direction).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

FVector FOverdriveCombatDetectorEditMode::GetDirectionArrowStart(const FOverdriveCombatImpactContext& Context) const
{
	// Origin 마커와 겹치지 않도록 방향으로 조금 띄운 지점. 화살표 그리기 / 라벨 / 기즈모 위치가 모두 여기를 기준으로 한다.
	return Context.WorldOrigin + GetDirectionWorld(Context) * DirectionArrowStartGap;
}

bool FOverdriveCombatDetectorEditMode::HasActiveTarget() const
{
	switch (SelectedHandle)
	{
	case EOverdriveCombatHandle::Origin:
		return HasOriginTarget();

	case EOverdriveCombatHandle::Direction:
		return HasDirectionTarget();

	default:
		return HasShapeTarget();
	}
}

UObject* FOverdriveCombatDetectorEditMode::GetActiveOwnerObject() const
{
	switch (SelectedHandle)
	{
	case EOverdriveCombatHandle::Origin:
		return GetOriginOwnerObject();

	case EOverdriveCombatHandle::Direction:
		// 스펙은 노티파이의 값 프로퍼티이므로 트랜잭션/PostEditChange 대상도 노티파이(=Origin 소유자)다.
		return GetOriginOwnerObject();

	default:
		return GetShapeOwnerObject();
	}
}

FName FOverdriveCombatDetectorEditMode::GetActiveTransformPropertyName() const
{
	switch (SelectedHandle)
	{
	case EOverdriveCombatHandle::Origin:
		return GetOriginPropertyName();

	case EOverdriveCombatHandle::Direction:
		return GetImpactNormalSpecPropertyName();

	default:
		return GetShapeTransformPropertyName();
	}
}

bool FOverdriveCombatDetectorEditMode::GetCameraTarget(FSphere& OutTarget) const
{
	if (!ShouldDrawWidget())
	{
		return false;
	}

	OutTarget.Center = GetWidgetLocation();
	OutTarget.W = 30.0f;

	return true;
}

void FOverdriveCombatDetectorEditMode::GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const
{
}

bool FOverdriveCombatDetectorEditMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	const EAxisList::Type CurrentAxis = (InViewportClient != nullptr) ? InViewportClient->GetCurrentWidgetAxis() : EAxisList::None;
	const UE::Widget::EWidgetMode WidgetMode = (InViewportClient != nullptr) ? InViewportClient->GetWidgetMode() : UE::Widget::WM_None;

	if (WidgetMode != UE::Widget::WM_None && CurrentAxis != EAxisList::None)
	{
		return HandleBeginTransform();
	}

	return false;
}

bool FOverdriveCombatDetectorEditMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return HandleEndTransform();
}

bool FOverdriveCombatDetectorEditMode::BeginTransform(const FGizmoState& InState)
{
	return HandleBeginTransform();
}

bool FOverdriveCombatDetectorEditMode::EndTransform(const FGizmoState& InState)
{
	return HandleEndTransform();
}

bool FOverdriveCombatDetectorEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HOverdriveCombatHandleProxy* HandleProxy = HitProxyCast<HOverdriveCombatHandleProxy>(HitProxy))
	{
		SelectedHandle = HandleProxy->Handle;
		EnsureSupportedWidgetMode();

		if (InViewportClient != nullptr)
		{
			InViewportClient->Invalidate();
		}

		return true;
	}

	return false;
}

void FOverdriveCombatDetectorEditMode::EnsureSupportedWidgetMode()
{
	if (Owner == nullptr || UsesTransformWidget(Owner->GetWidgetMode()))
	{
		return;
	}

	// 핸들별 허용 모드가 다르므로(Origin=이동, Direction=회전, Shape=이동+회전)
	// 전환 직후 못 쓰는 모드로 남아 있으면 기즈모가 사라진다. 쓸 수 있는 첫 모드로 스냅한다.
	const UE::Widget::EWidgetMode Candidates[] = { UE::Widget::WM_Translate, UE::Widget::WM_Rotate };
	for (const UE::Widget::EWidgetMode Candidate : Candidates)
	{
		if (UsesTransformWidget(Candidate))
		{
			Owner->SetWidgetMode(Candidate);
			return;
		}
	}
}

bool FOverdriveCombatDetectorEditMode::HandleBeginTransform()
{
	UObject* OwnerObject = GetActiveOwnerObject();
	if (OwnerObject == nullptr)
	{
		return false;
	}

	if (!bInTransaction)
	{
		GEditor->BeginTransaction(LOCTEXT("EditDetectorHandle", "Edit Overdrive Combat Handle"));

		OwnerObject->SetFlags(RF_Transactional);	// 이 플래그가 없으면 Undo 가 동작하지 않는다.
		OwnerObject->Modify();

		if (UAnimSequenceBase* AnimAsset = TargetAnimAsset.Get())
		{
			// 소유 애님 에셋도 트랜잭션에 넣어 패키지 더티와 Undo 복원을 보장한다.
			AnimAsset->Modify();
		}

		bInTransaction = true;
	}

	bManipulating = true;

	return true;
}

bool FOverdriveCombatDetectorEditMode::HandleEndTransform()
{
	if (!bManipulating)
	{
		return false;
	}

	bManipulating = false;

	if (bInTransaction)
	{
		GEditor->EndTransaction();
		bInTransaction = false;
	}

	if (UObject* OwnerObject = GetActiveOwnerObject())
	{
		// 디테일 패널이 드래그 결과를 반영하도록 프로퍼티 변경을 브로드캐스트한다.
		if (FProperty* Property = FindFProperty<FProperty>(OwnerObject->GetClass(), GetActiveTransformPropertyName()))
		{
			FPropertyChangedEvent ChangedEvent(Property, EPropertyChangeType::ValueSet);
			OwnerObject->PostEditChangeProperty(ChangedEvent);
		}
	}

	return true;
}

bool FOverdriveCombatDetectorEditMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	const EAxisList::Type CurrentAxis = InViewportClient->GetCurrentWidgetAxis();
	const UE::Widget::EWidgetMode WidgetMode = InViewportClient->GetWidgetMode();

	if (!bManipulating || CurrentAxis == EAxisList::None || GetPreviewMeshComponent() == nullptr)
	{
		return false;
	}

	bool bApplied = false;

	switch (SelectedHandle)
	{
	case EOverdriveCombatHandle::Origin:
		bApplied = ApplyOriginDelta(InDrag, WidgetMode);
		break;

	case EOverdriveCombatHandle::Direction:
		bApplied = ApplyDirectionDelta(InRot, WidgetMode);
		break;

	default:
		bApplied = ApplyShapeDelta(InDrag, InRot, WidgetMode);
		break;
	}

	if (!bApplied)
	{
		return false;
	}

	InViewport->Invalidate();

	return true;
}

bool FOverdriveCombatDetectorEditMode::ApplyOriginDelta(const FVector& InDrag, UE::Widget::EWidgetMode WidgetMode)
{
	// 원점은 위치만 편집한다(회전/스케일 위젯 없음).
	if (!HasOriginTarget() || WidgetMode != UE::Widget::WM_Translate)
	{
		return false;
	}

	const FTransform ComponentToWorld = GetPreviewMeshComponent()->GetComponentTransform();
	const FVector WorldLocation = ComponentToWorld.TransformPosition(GetOriginLocal()) + InDrag;
	SetOriginLocal(ComponentToWorld.InverseTransformPosition(WorldLocation));

	return true;
}

bool FOverdriveCombatDetectorEditMode::ApplyShapeDelta(const FVector& InDrag, const FRotator& InRot, UE::Widget::EWidgetMode WidgetMode)
{
	if (!HasShapeTarget())
	{
		return false;
	}

	// RelativeTransform 은 부모 프레임 기준 오프셋이므로, 역변환 기준도 그 프레임의 월드다.
	const FTransform AnchorToWorld = GetShapeAnchorToWorld();
	FTransform WorldTransform = GetShapeRelativeTransform() * AnchorToWorld;

	if (WidgetMode == UE::Widget::WM_Rotate)
	{
		WorldTransform.SetRotation((InRot.Quaternion() * WorldTransform.GetRotation()).GetNormalized());
	}

	if (WidgetMode == UE::Widget::WM_Translate)
	{
		WorldTransform.AddToTranslation(InDrag);
	}

	// SetShapeRelativeTransform 이 스케일을 (1,1,1)로 강제하므로 역변환 잔여 스케일이 남지 않는다.
	SetShapeRelativeTransform(WorldTransform.GetRelativeTransform(AnchorToWorld));

	return true;
}

bool FOverdriveCombatDetectorEditMode::ApplyDirectionDelta(const FRotator& InRot, UE::Widget::EWidgetMode WidgetMode)
{
	// 방향은 회전만 편집한다.
	if (!HasDirectionTarget() || WidgetMode != UE::Widget::WM_Rotate)
	{
		return false;
	}

	FOverdriveCombatImpactNormalSpec* Spec = GetFixedDirectionSpec();
	const FTransform ComponentToWorld = MakeImpactContext().ComponentToWorld;

	// 월드에서 회전시킨 뒤 컴포넌트 공간으로 되돌린다(기즈모 축은 월드 기준이다).
	const FVector CurrentWorld = ComponentToWorld.TransformVectorNoScale(Spec->Direction);
	const FVector RotatedWorld = InRot.Quaternion().RotateVector(CurrentWorld);
	const FVector NewLocal = ComponentToWorld.InverseTransformVectorNoScale(RotatedWorld).GetSafeNormal();

	// 0 길이로 무너지면 기존 방향을 유지한다.
	if (!NewLocal.IsNearlyZero())
	{
		Spec->Direction = NewLocal;
	}

	return true;
}

bool FOverdriveCombatDetectorEditMode::AllowWidgetMove()
{
	return ShouldDrawWidget();
}

bool FOverdriveCombatDetectorEditMode::ShouldDrawWidget() const
{
	return GetPreviewMeshComponent() != nullptr && HasActiveTarget();
}

bool FOverdriveCombatDetectorEditMode::UsesTransformWidget() const
{
	return ShouldDrawWidget();
}

bool FOverdriveCombatDetectorEditMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	if (!ShouldDrawWidget())
	{
		return false;
	}

	if (SelectedHandle == EOverdriveCombatHandle::Origin)
	{
		// 원점은 위치만. 스케일은 어느 핸들에서도 셰이프 치수 프로퍼티로만 조절한다.
		return CheckMode == UE::Widget::WM_Translate;
	}

	if (SelectedHandle == EOverdriveCombatHandle::Direction)
	{
		// 방향은 회전만.
		return CheckMode == UE::Widget::WM_Rotate;
	}

	return CheckMode == UE::Widget::WM_Translate || (ShapeUsesRotation() && CheckMode == UE::Widget::WM_Rotate);
}

FVector FOverdriveCombatDetectorEditMode::GetWidgetLocation() const
{
	if (SelectedHandle == EOverdriveCombatHandle::Origin && HasOriginTarget())
	{
		if (const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent())
		{
			return PreviewMeshComponent->GetComponentTransform().TransformPosition(GetOriginLocal());
		}
	}

	if (SelectedHandle == EOverdriveCombatHandle::Direction && HasDirectionTarget())
	{
		// 기즈모도 화살표와 함께 띄운다. 회전은 델타 회전만 적용하므로(ApplyDirectionDelta)
		// 위젯이 Origin 을 벗어나도 결과는 같고, 기즈모 원이 Origin 마커를 덮지 않는다.
		return GetDirectionArrowStart(MakeImpactContext());
	}

	return GetShapeWorldTransform().GetLocation();
}

bool FOverdriveCombatDetectorEditMode::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (SelectedHandle == EOverdriveCombatHandle::Origin)
	{
		// 원점 로컬 좌표계는 컴포넌트 회전에 정렬한다.
		const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
		if (!HasOriginTarget() || PreviewMeshComponent == nullptr)
		{
			return false;
		}

		InMatrix = PreviewMeshComponent->GetComponentTransform().ToMatrixNoScale().RemoveTranslation();
		return true;
	}

	if (SelectedHandle == EOverdriveCombatHandle::Direction)
	{
		if (!HasDirectionTarget())
		{
			return false;
		}

		// 위젯 X 축이 화살표를 따라가게 한다.
		InMatrix = FRotationMatrix::MakeFromX(GetDirectionWorld(MakeImpactContext()));

		return true;
	}

	if (!HasShapeTarget())
	{
		return false;
	}

	// 로컬 좌표계 모드에서 위젯 축이 셰이프 회전을 따르게 한다.
	InMatrix = GetShapeWorldTransform().ToMatrixNoScale().RemoveTranslation();

	return true;
}

bool FOverdriveCombatDetectorEditMode::GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	return GetCustomDrawingCoordinateSystem(InMatrix, InData);
}

void FOverdriveCombatDetectorEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	IPersonaEditMode::Render(View, Viewport, PDI);

	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	if (PreviewMeshComponent == nullptr)
	{
		return;
	}

	// 히트 프록시 패스가 아닐 때는 프록시를 만들지 않는다. 일반 렌더링에서는 소비자가 없어
	// 만들자마자 버려지므로(엔진 에디트 모드 관례) 매 프레임 낭비되는 할당을 막는다.
	const bool bHitTesting = PDI->IsHitTesting();

	// 셰이프: 와이어 + 클릭 픽업용 마커. 선택 중이면 강조색.
	if (HasShapeTarget())
	{
		const bool bShapeSelected = (SelectedHandle == EOverdriveCombatHandle::Shape);
		const FLinearColor Color = bShapeSelected ? ShapeSelectedColor : ShapeColor;

		RenderShape(PDI, Color);

		if (bHitTesting)
		{
			PDI->SetHitProxy(new HOverdriveCombatHandleProxy(EOverdriveCombatHandle::Shape));
		}

		DrawWireSphere(PDI, GetShapeWorldTransform().GetLocation(), Color, HandleMarkerRadius, 12, SDPG_Foreground);

		if (bHitTesting)
		{
			PDI->SetHitProxy(nullptr);
		}
	}

	// Origin: 마커 + 십자선. 선택 중이면 강조색.
	if (HasOriginTarget())
	{
		const bool bOriginSelected = (SelectedHandle == EOverdriveCombatHandle::Origin);
		const FLinearColor Color = bOriginSelected ? OriginSelectedColor : OriginColor;
		const FVector OriginLocation = PreviewMeshComponent->GetComponentTransform().TransformPosition(GetOriginLocal());

		if (bHitTesting)
		{
			PDI->SetHitProxy(new HOverdriveCombatHandleProxy(EOverdriveCombatHandle::Origin));
		}

		DrawWireSphere(PDI, OriginLocation, Color, HandleMarkerRadius, 12, SDPG_Foreground);
		PDI->DrawLine(OriginLocation - FVector(HandleMarkerRadius * 2.0f, 0.0f, 0.0f), OriginLocation + FVector(HandleMarkerRadius * 2.0f, 0.0f, 0.0f), Color, SDPG_Foreground);
		PDI->DrawLine(OriginLocation - FVector(0.0f, HandleMarkerRadius * 2.0f, 0.0f), OriginLocation + FVector(0.0f, HandleMarkerRadius * 2.0f, 0.0f), Color, SDPG_Foreground);
		PDI->DrawLine(OriginLocation - FVector(0.0f, 0.0f, HandleMarkerRadius * 2.0f), OriginLocation + FVector(0.0f, 0.0f, HandleMarkerRadius * 2.0f), Color, SDPG_Foreground);

		if (bHitTesting)
		{
			PDI->SetHitProxy(nullptr);
		}
	}

	// Direction: 화살표. 선택 중이면 강조색.
	if (HasDirectionTarget())
	{
		const bool bDirectionSelected = (SelectedHandle == EOverdriveCombatHandle::Direction);
		const FLinearColor Color = bDirectionSelected ? DirectionSelectedColor : DirectionColor;
		const FOverdriveCombatImpactContext Context = MakeImpactContext();

		if (bHitTesting)
		{
			PDI->SetHitProxy(new HOverdriveCombatHandleProxy(EOverdriveCombatHandle::Direction));
		}

		// DrawDirectionalArrow 는 +X 방향으로 그리므로 X 축을 방향에 정렬한다.
		// 기점은 Origin 에서 조금 띄운 지점이라 밑동이 Origin 마커의 클릭을 가리지 않는다.
		const FMatrix ArrowToWorld = FRotationMatrix::MakeFromX(GetDirectionWorld(Context)) * FTranslationMatrix(GetDirectionArrowStart(Context));
		DrawDirectionalArrow(PDI, ArrowToWorld, Color, DirectionArrowLength, DirectionArrowHeadSize, SDPG_Foreground, DirectionArrowThickness);

		if (bHitTesting)
		{
			PDI->SetHitProxy(nullptr);
		}
	}
}

void FOverdriveCombatDetectorEditMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	IPersonaEditMode::DrawHUD(ViewportClient, Viewport, View, Canvas);

	const UDebugSkelMeshComponent* PreviewMeshComponent = GetPreviewMeshComponent();
	if (Viewport == nullptr || View == nullptr || Canvas == nullptr || PreviewMeshComponent == nullptr)
	{
		return;
	}

	if (HasShapeTarget())
	{
		const bool bShapeSelected = (SelectedHandle == EOverdriveCombatHandle::Shape);
		DrawHandleLabel(Viewport, View, Canvas, GetShapeWorldTransform().GetLocation(), GetShapeLabel(), bShapeSelected ? ShapeSelectedColor : ShapeColor);
	}

	if (HasOriginTarget())
	{
		const bool bOriginSelected = (SelectedHandle == EOverdriveCombatHandle::Origin);
		const FVector OriginLocation = PreviewMeshComponent->GetComponentTransform().TransformPosition(GetOriginLocal());
		DrawHandleLabel(Viewport, View, Canvas, OriginLocation, OriginLabel, bOriginSelected ? OriginSelectedColor : OriginColor);
	}

	if (HasDirectionTarget())
	{
		const bool bDirectionSelected = (SelectedHandle == EOverdriveCombatHandle::Direction);
		const FOverdriveCombatImpactContext Context = MakeImpactContext();

		// Origin 라벨과 겹치지 않도록 화살표 끝에 띄운다.
		const FVector TipLocation = GetDirectionArrowStart(Context) + GetDirectionWorld(Context) * DirectionArrowLength;
		DrawHandleLabel(Viewport, View, Canvas, TipLocation, DirectionLabel, bDirectionSelected ? DirectionSelectedColor : DirectionColor);
	}
}

void FOverdriveCombatDetectorEditMode::DrawHandleLabel(FViewport* Viewport, const FSceneView* View, FCanvas* Canvas, const FVector& WorldLocation, const FString& Label, const FLinearColor& Color) const
{
	UFont* Font = (GEngine != nullptr) ? GEngine->GetSmallFont() : nullptr;
	if (Font == nullptr || Label.IsEmpty())
	{
		return;
	}

	// W <= 0 이면 카메라 뒤라 화면에 없다.
	const FPlane Projection = View->Project(WorldLocation);
	if (Projection.W <= 0.0f)
	{
		return;
	}

	// 캔버스는 DPI 스케일 좌표계라 뷰포트 픽셀 크기를 나눠서 반값을 구한다(엔진 에디트 모드 관례).
	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	const float DPIScale = Canvas->GetDPIScale();
	const float HalfX = 0.5f * (static_cast<float>(ViewportSize.X) / DPIScale);
	const float HalfY = 0.5f * (static_cast<float>(ViewportSize.Y) / DPIScale);

	const float ScreenX = HalfX + (HalfX * static_cast<float>(Projection.X));
	const float ScreenY = HalfY + (HalfY * -static_cast<float>(Projection.Y));

	FCanvasTextItem TextItem(FVector2D(ScreenX + LabelScreenOffsetX, ScreenY + LabelScreenOffsetY), FText::FromString(Label), Font, Color);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
}

#undef LOCTEXT_NAMESPACE
