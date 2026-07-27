// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
/**
 *
 */

namespace OverdriveCombatTags
{
	// UE_DECLARE_GAMEPLAY_TAG_EXTERN 은 `extern FNativeGameplayTag TagName;` 로 확장된다(NativeGameplayTags.h:31).
	// 앞에 모듈 API 매크로를 붙여야 다른 모듈에서 이 태그를 링크할 수 있다.
	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_Hit);

	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_Damage);
	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_Dead);
	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Event_OnHit);

	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Attack);

	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_SetByCaller_Damage);

	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_HitStop);
	OVERDRIVECOMBAT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dead);

}
