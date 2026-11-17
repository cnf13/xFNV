#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

class CombatController;

// The combat action that attacks with a melee weapon (the PDB defines it in CombatActions.cpp). Only the entry
// points the reconstructed code defines are declared.
class CombatActionAttackMelee : public CombatAction {
public:
	CombatActionAttackMelee(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the melee weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackMelee_vftable{};
