// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilities/OverdriveCombatTargetData_AttackHit.h"

bool FOverdriveCombatTargetData_AttackHit::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// 부모 NetSerialize 는 비가상이라 명시적으로 호출해 HitResult 를 먼저 직렬화한다.
	FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);

	AttackTypeTag.NetSerialize(Ar, Map, bOutSuccess);
	Origin.NetSerialize(Ar, Map, bOutSuccess);

	return true;
}
