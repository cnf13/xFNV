#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

class CombatController;

// The combat action that fires a ranged weapon. It adds no fields to CombatAction.
class CombatActionAttackRanged : public CombatAction {
public:
	CombatActionAttackRanged(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the ranged weapon cost, rounded to float; the most a float holds for a player teammate whose
	// CombatStyleMelee variable is 1.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
