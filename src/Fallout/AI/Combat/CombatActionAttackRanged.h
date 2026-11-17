#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

class CombatController;

// The combat action that fires a ranged weapon (the PDB defines it in CombatActions.cpp). It adds no fields to
// CombatAction.
class CombatActionAttackRanged : public CombatAction {
public:
	CombatActionAttackRanged(CombatAction::COMBAT_ACTION aeType);

	// thiscall, ret 8, 0x97B170; IDA names it AddProcedureToController.
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the ranged weapon cost, rounded to float; the most a float holds for a player teammate whose
	// CombatStyleMelee variable is 1.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackRanged_vftable{};
