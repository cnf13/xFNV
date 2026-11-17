#pragma once

#include "Fallout/AI/Combat/CombatProcedure.h"

class TESObjectWEAP;

// The combat procedure that equips another weapon: the weapon to switch to and whether the equip is queued.
class CombatProcedureSwitchWeapon : public CombatProcedure {
public:
	CombatProcedureSwitchWeapon(TESObjectWEAP* apWeapon);
	~CombatProcedureSwitchWeapon() override;

	void Update() override;
	CombatProcedure::COMBAT_PROCEDURES GetType() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	TESObjectWEAP* pWeapon;
	bool bEquipQueued;
	uint8_t padding15[3];
};
