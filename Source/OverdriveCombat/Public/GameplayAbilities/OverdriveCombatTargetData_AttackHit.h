// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"
#include "GameplayTagContainer.h"
#include "OverdriveCombatTargetData_AttackHit.generated.h"

/**
 * 공격 타입 태그가 붙은 단일 히트 TargetData.
 *
 * 히트 판정 디텍터(UOverdriveCombatHitBurstDetector)가 히트마다 하나씩 만들어
 * FGameplayAbilityTargetDataHandle 에 담고, GameplayEvent 로 수신 어빌리티까지 전달된다.
 *
 * NetSerialize 는 가상 함수가 아니라 GetScriptStruct() + TStructOpsTypeTraits 로
 * 다형 디스패치되므로, 파생 구조체마다 셋 다 다시 정의해야 한다(엔진 GameplayAbilityTargetTypes.h 관례).
 */
USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/OverdriveCombat.OverdriveCombatLibrary.BreakAttackHitTargetData"))
struct OVERDRIVECOMBAT_API FOverdriveCombatTargetData_AttackHit : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	FOverdriveCombatTargetData_AttackHit() = default;

	FOverdriveCombatTargetData_AttackHit(FHitResult InHitResult, const FGameplayTag& InAttackTypeTag)
		: FGameplayAbilityTargetData_SingleTargetHit(MoveTemp(InHitResult))
		, AttackTypeTag(InAttackTypeTag)
	{
	}

	/** 이 히트를 만든 공격의 타입. 디텍터 인스턴스에 설정된 값이 복사된다. */
	UPROPERTY(BlueprintReadOnly, Category = "OverdriveCombat")
	FGameplayTag AttackTypeTag;

	/**
	 * 이 히트를 낳은 공격의 월드 원점. 넉백/파동 중심으로 쓴다. 노티파이가 전송 직전 후처리로 채운다.
	 * FVector_NetQuantize10 은 BlueprintType 이 아니므로 UPROPERTY() 로만 두고(BlueprintReadOnly 금지),
	 * 노출은 아래 GetOrigin() 오버라이드로 엔진 표준 API 에 얹는다.
	 */
	UPROPERTY()
	FVector_NetQuantize10 Origin = FVector_NetQuantize10(ForceInitToZero);

	/** 원점을 갖는다. 베이스는 트레이스에서 유도하지만 이 타입은 저장된 Origin 을 쓴다. */
	virtual bool HasOrigin() const override { return true; }

	/** 저장된 공격 원점을 트랜스폼(위치만)으로 돌려준다. 수신 어빌리티가 표준 GetOrigin() 으로 읽는다. */
	virtual FTransform GetOrigin() const override { return FTransform(FVector(Origin)); }

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FOverdriveCombatTargetData_AttackHit::StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FOverdriveCombatTargetData_AttackHit> : public TStructOpsTypeTraitsBase2<FOverdriveCombatTargetData_AttackHit>
{
	enum
	{
		// FGameplayAbilityTargetDataHandle 넷 직렬화에 필수.
		WithNetSerializer = true
	};
};
