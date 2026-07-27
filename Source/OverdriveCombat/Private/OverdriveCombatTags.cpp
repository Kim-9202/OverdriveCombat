// Fill out your copyright notice in the Description page of Project Settings.


#include "OverdriveCombatTags.h"

namespace OverdriveCombatTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_Hit, "Combat.Event.Hit", "애님 노티파이 스윕이 대상을 맞췄을 때 공격자에게 전송되는 GameplayEvent. 페이로드의 TargetData 에 히트별 FHitResult 가 담긴다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_Damage, "Combat.Event.Damage", "데미지 적용 후 행동을 요청하는 GameplayEvent.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_Dead, "Combat.Event.Dead", "죽었을 시에 대한 행동을 요청하는 GameplayEvent.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Event_OnHit, "Combat.Event.OnHit", "공격에 성공하고 DamageEffect를 적용 성공한 공격자에게 Event를 보내기 위한 Tag");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Attack, "Combat.Attack", "공격 타입 태그의 루트. 하위 태그로 공격 타입을 정의하고 디텍터의 AttackTypeTag 에 지정한다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_SetByCaller_Damage, "Combat.SetByCaller.Damage", "데미지 GE 스펙에 어빌리티가 싣는 기본 데미지 값(SetByCaller). UOverdriveCombatEffectExecution_Damage 가 읽는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_HitStop, "Character.State.HitStop", "히트스톱이 적용된 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dead, "Character.State.Dead", "죽은 상태.");
}
