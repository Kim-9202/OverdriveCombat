// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotifies/OverdriveCombatHitDetectionTypes.h"
#include "OverdriveCombatAnimNotify_Attack.generated.h"

class UOverdriveCombatHitBurstDetector;

/**
 * 단발 공격 판정 노티파이.
 *
 * 그 순간의 포즈에서 디텍터가 히트 판정과 TargetData 패킹을 수행하고,
 * 노티파이는 채워진 FGameplayAbilityTargetDataHandle 로 공격자에게 GameplayEvent 만 보낸다.
 * 서버 권위에서만 동작한다.
 *
 * 애님 에디터 프리뷰에서는 물리 판정도 이벤트 전송도 하지 않고 셰이프만 디버그 드로우한다(od.Combat.DrawHitDetection).
 *
 * 프레임 간 상태가 전혀 없으므로 노티파이 인스턴스 공유 문제에서 자유롭다.
 */
UCLASS(EditInlineNew, Const, HideCategories = Object, CollapseCategories, Meta = (DisplayName = "Overdrive Combat Attack (Burst)"))
class OVERDRIVECOMBAT_API UOverdriveCombatAnimNotify_Attack : public UAnimNotify
{
	GENERATED_BODY()

public:
	UOverdriveCombatAnimNotify_Attack(const FObjectInitializer& ObjectInitializer);

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** 에디터(기즈모 편집)가 쓰는 디텍터 접근자. */
	UOverdriveCombatHitBurstDetector* GetHitDetector() const { return HitDetector; }

	/** 컴포넌트 상대 공격 원점(런타임 후처리·기즈모 편집이 쓴다). */
	FVector GetAttackOrigin() const { return AttackOrigin; }

	/** ImpactNormal 재계산 스펙(런타임 후처리가 쓴다). */
	const FOverdriveCombatImpactNormalSpec& GetImpactNormalSpec() const { return ImpactNormalSpec; }

#if WITH_EDITOR
	/** 기즈모가 드래그한 컴포넌트 상대 원점을 반영한다. */
	void SetAttackOrigin(const FVector& InLocal) { AttackOrigin = InLocal; }

	/** 기즈모 편집용 스펙 접근자(런타임 경로는 const getter 를 쓴다). */
	FOverdriveCombatImpactNormalSpec& GetImpactNormalSpecForEdit() { return ImpactNormalSpec; }

	/** AttackOrigin 이 private 이므로 에디터 모듈이 PostEditChangeProperty 에 쓰는 프로퍼티 이름 통로. */
	static FName GetAttackOriginPropertyName();

	/** ImpactNormalSpec 이 private 이므로 에디터 모듈이 PostEditChangeProperty 에 쓰는 프로퍼티 이름 통로. */
	static FName GetImpactNormalSpecPropertyName();

	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;

	/** 로드·저장 시점의 설정 점검 훅. 디텍터가 비어 있으면 AssetCheck 메시지 로그로 경고한다. */
	virtual void ValidateAssociatedAssets() override;
#endif

private:
	/** 히트 판정 + TargetData 패킹 정책. */
	UPROPERTY(EditAnywhere, Instanced, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOverdriveCombatHitBurstDetector> HitDetector;

	/** 공격자에게 전송할 GameplayEvent 태그. 기본값은 Combat.Event.Hit 이고, 지워서 비우면 전송 시 그 기본 태그로 대체된다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true", Categories = "Combat.Event"))
	FGameplayTag EventTag;

	/** 컴포넌트 상대 공격 원점. 기즈모로 편집하며, 히트마다 TargetData 에 실려 넉백/파동 중심으로 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	FVector AttackOrigin = FVector::ZeroVector;

	/** ImpactNormal 재계산 스펙. Mode 가 None 이면 히트가 준 노멀을 그대로 둔다. */
	UPROPERTY(EditAnywhere, Category = "OverdriveCombat", meta = (AllowPrivateAccess = "true"))
	FOverdriveCombatImpactNormalSpec ImpactNormalSpec;

};
