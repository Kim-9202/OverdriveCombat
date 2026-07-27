// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"

/** 에디트 모드가 한 뷰포트에서 다루는 세 편집 핸들. */
enum class EOverdriveCombatHandle : uint8
{
	/** 디텍터 셰이프의 배치 트랜스폼. */
	Shape,

	/** 노티파이의 컴포넌트 상대 공격 원점. */
	Origin,

	/** ImpactNormal Modifier 의 편집 가능한 방향(회전 전용). */
	Direction
};

/**
 * 뷰포트에서 셰이프/Origin/Direction 마커를 클릭해 편집 대상을 전환하기 위한 히트 프록시.
 * FEdMode::HandleClick 이 이 프록시를 받아 선택 핸들을 바꾼다.
 */
struct HOverdriveCombatHandleProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	EOverdriveCombatHandle Handle;

	explicit HOverdriveCombatHandleProxy(EOverdriveCombatHandle InHandle)
		: HHitProxy(HPP_UI)
		, Handle(InHandle)
	{
	}
};
