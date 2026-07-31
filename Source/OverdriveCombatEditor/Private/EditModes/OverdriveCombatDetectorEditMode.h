// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IPersonaEditMode.h"
#include "UnrealWidgetFwd.h"
#include "EditModes/OverdriveCombatHandleProxy.h"

class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class UAnimSequenceBase;
class UDebugSkelMeshComponent;
struct FGizmoState;
struct FOverdriveCombatImpactContext;
struct FOverdriveCombatImpactNormalSpec;
struct FViewportClick;

/**
 * Attack 노티파이의 세 편집 대상(디텍터 셰이프 트랜스폼 + 컴포넌트 상대 공격 원점 +
 * ImpactNormal 스펙의 방향)을 한 Persona 뷰포트 기즈모로 편집하는 공용 베이스 에디트 모드.
 *
 * Direction 핸들은 스펙의 Mode 가 FixedDirection 일 때만 활성화되어 Direction 프로퍼티를
 * 편집한다. 월드 변환·화살표 그리기·회전 적용·클릭 픽업 등 방향 편집의 에디터 로직은
 * 전부 이 클래스에 있다(런타임 모듈에 에디터 코드를 두지 않는다).
 *
 * 뷰포트에 각 핸들 위치로 HHitProxy 마커를 그려, 클릭으로 편집 대상을 전환한다(HandleClick).
 * 트랜잭션/좌표계/신형·레거시 기즈모 통합 로직은 모두 여기에 두고, 셰이프 배치와 원점 접근처럼
 * 노티파이 구체 타입에 의존하는 부분만 파생이 protected 훅으로 채운다.
 *
 * Persona 는 신형 기즈모 경로를 쓰므로(BeginTransform / EndTransform) 레거시
 * StartTracking / EndTracking 과 함께 HandleBeginTransform / HandleEndTransform 으로 통합한다
 * (엔진 FAnimNodeEditMode 관례).
 */
class FOverdriveCombatDetectorEditMode : public IPersonaEditMode
{
public:
	/** IPersonaEditMode interface */
	virtual bool GetCameraTarget(FSphere& OutTarget) const override;
	virtual class IPersonaPreviewScene& GetAnimPreviewScene() const override;
	virtual void GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const override;

	/** FEdMode interface */
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool BeginTransform(const FGizmoState& InState) override;
	virtual bool EndTransform(const FGizmoState& InState) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool AllowWidgetMove() override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override;
	virtual FVector GetWidgetLocation() const override;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual bool GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;

protected:
	// ---- 파생이 채우는 셰이프 훅 (디텍터 구체 타입 의존) ----

	/** 편집할 디텍터가 유효한가. */
	virtual bool HasShapeTarget() const = 0;

	/** RelativeTransform 의 부모 프레임(월드). Burst=컴포넌트, Sweep=앵커소켓 x 컴포넌트. */
	virtual FTransform GetShapeAnchorToWorld() const = 0;

	/** 디텍터의 배치 오프셋(부모 프레임 상대). */
	virtual FTransform GetShapeRelativeTransform() const = 0;

	/** 디텍터의 배치 오프셋을 설정한다(스케일은 디텍터가 1 로 강제). */
	virtual void SetShapeRelativeTransform(const FTransform& InTransform) = 0;

	/** 셰이프 기즈모에 회전 위젯을 허용하는가. */
	virtual bool ShapeUsesRotation() const = 0;

	/** 편집 중 강조색으로 셰이프 와이어를 그린다. */
	virtual void RenderShape(FPrimitiveDrawInterface* PDI, const FLinearColor& Color) const = 0;

	/** 트랜잭션 / PostEditChangeProperty 대상(디텍터). */
	virtual UObject* GetShapeOwnerObject() const = 0;

	/** 디텍터의 RelativeTransform 프로퍼티 이름. */
	virtual FName GetShapeTransformPropertyName() const = 0;

	/** 셰이프 핸들 옆에 띄울 이름(보통 디텍터 표시 이름). */
	virtual FString GetShapeLabel() const = 0;

	// ---- 파생이 채우는 Origin 훅 (노티파이 구체 타입 의존) ----

	/** 편집할 노티파이(원점 소유)가 유효한가. */
	virtual bool HasOriginTarget() const = 0;

	/** 컴포넌트 상대 공격 원점. */
	virtual FVector GetOriginLocal() const = 0;

	/** 컴포넌트 상대 공격 원점을 설정한다. */
	virtual void SetOriginLocal(const FVector& InLocal) = 0;

	/** 트랜잭션 / PostEditChangeProperty 대상(노티파이). */
	virtual UObject* GetOriginOwnerObject() const = 0;

	/** 노티파이의 AttackOrigin 프로퍼티 이름. */
	virtual FName GetOriginPropertyName() const = 0;

	// ---- 파생이 채우는 Direction 훅 (노티파이 구체 타입 의존) ----

	/**
	 * 편집 대상 노티파이의 ImpactNormal 스펙(노티파이가 죽었으면 nullptr).
	 * 편집이 필요하므로 non-const 포인터. 노티파이 재인스턴싱에 대비해 프레임 간 저장 금지 —
	 * 매 사용마다 이 훅으로 다시 얻는다.
	 */
	virtual FOverdriveCombatImpactNormalSpec* GetImpactNormalSpec() const = 0;

	/** 노티파이의 ImpactNormalSpec 프로퍼티 이름(PostEditChangeProperty 용). */
	virtual FName GetImpactNormalSpecPropertyName() const = 0;

	// ---- 공용 헬퍼 ----

	/** 셰이프의 월드 트랜스폼(오프셋 x 부모 프레임). */
	FTransform GetShapeWorldTransform() const;

	/** 프리뷰 메시 컴포넌트(없으면 nullptr). */
	UDebugSkelMeshComponent* GetPreviewMeshComponent() const;

	/** 디텍터를 소유한 애님 에셋. 트랜잭션 / 패키지 더티 대상. 파생 SetTarget 이 채운다. */
	TWeakObjectPtr<UAnimSequenceBase> TargetAnimAsset;

	/** 현재 편집 중인 핸들. HandleClick 으로 전환된다. */
	EOverdriveCombatHandle SelectedHandle = EOverdriveCombatHandle::Shape;

private:
	/** 레거시(StartTracking)와 신형 기즈모(BeginTransform) 공용 트랜잭션 시작. */
	bool HandleBeginTransform();

	/** 레거시(EndTracking)와 신형 기즈모(EndTransform) 공용 트랜잭션 종료 + 디테일 리프레시. */
	bool HandleEndTransform();

	/**
	 * 프리뷰 기준 컨텍스트(Origin 월드 위치 + 컴포넌트 트랜스폼).
	 * 런타임 호출부(OverdriveCombatAnimNotify_Attack::Notify)와 같은 값을 채우므로
	 * 뷰포트에서 편집한 방향이 실제 히트 결과와 일치한다. 프리뷰 메시가 없으면 Identity 기준.
	 */
	FOverdriveCombatImpactContext MakeImpactContext() const;

	/** Direction 핸들이 실제로 편집 가능한 대상을 갖는가. */
	bool HasDirectionTarget() const;

	/** 방향 편집을 지원하는 스펙(Mode == FixedDirection). 아니면 nullptr. */
	FOverdriveCombatImpactNormalSpec* GetFixedDirectionSpec() const;

	/**
	 * 편집 방향의 월드 벡터(정규화). ModifyImpactNormal 과 같은 변환식이라
	 * 뷰포트 화살표와 실제 넉백 방향이 어긋나지 않는다. 기즈모 좌표계의 축이 된다.
	 */
	FVector GetDirectionWorld(const FOverdriveCombatImpactContext& Context) const;

	/**
	 * 방향 화살표의 시작 지점(월드). Origin 마커에서 방향으로 조금 띄운 곳이라
	 * 화살표의 히트 프록시가 Origin 프록시를 덮지 않는다. 화살표 / 라벨 / 회전 기즈모가 공유한다.
	 */
	FVector GetDirectionArrowStart(const FOverdriveCombatImpactContext& Context) const;

	/**
	 * 선택 핸들이 바뀐 뒤 전역 위젯 모드가 그 핸들에서 못 쓰는 모드면 쓸 수 있는 모드로 스냅한다.
	 * 핸들마다 허용 모드가 달라(Origin=이동, Direction=회전) 그냥 두면 기즈모가 통째로
	 * 사라져 편집이 막힌 것처럼 보인다.
	 */
	void EnsureSupportedWidgetMode();

	/** 핸들별 InputDelta 처리. 각각 성공 시 true. */
	bool ApplyOriginDelta(const FVector& InDrag, UE::Widget::EWidgetMode WidgetMode);
	bool ApplyShapeDelta(const FVector& InDrag, const FRotator& InRot, UE::Widget::EWidgetMode WidgetMode);
	bool ApplyDirectionDelta(const FRotator& InRot, UE::Widget::EWidgetMode WidgetMode);

	/** 현재 선택 핸들의 트랜잭션 대상 오브젝트. */
	UObject* GetActiveOwnerObject() const;

	/** 현재 선택 핸들의 프로퍼티 이름. */
	FName GetActiveTransformPropertyName() const;

	/** 현재 선택 핸들이 실제로 편집 가능한 대상을 갖는가. */
	bool HasActiveTarget() const;

	/** 월드 위치를 화면으로 투영해 그 옆에 라벨을 그린다. 카메라 뒤면 아무것도 하지 않는다. */
	void DrawHandleLabel(FViewport* Viewport, const FSceneView* View, FCanvas* Canvas, const FVector& WorldLocation, const FString& Label, const FLinearColor& Color) const;

	/** 위젯을 드래그하는 중인지. */
	bool bManipulating = false;

	/** 트랜잭션이 열려 있는지. */
	bool bInTransaction = false;
};
