#pragma once

#include "BSMenu/IDebugText.h"
#include "Fallout/AI/Combat/CombatProcedure.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectWEAP;

// The combat procedure that attacks with a weapon; it tracks the weapon's DPS and condition.
class CombatProcedureAttackLow : public CombatProcedure {
public:
	enum ATTACK_STATE : int32_t {
		ATTACK_STATE_INITIALIZING = 0x0,
		ATTACK_STATE_ATTACKING = 0x1,
	};

	CombatProcedureAttackLow();
	~CombatProcedureAttackLow() override;

	void Update() override;
	void DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify) override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	ATTACK_STATE eState;
	float fWeaponDPS;
	float fWeaponCondition;
	TESObjectWEAP* pWeapon;
};
