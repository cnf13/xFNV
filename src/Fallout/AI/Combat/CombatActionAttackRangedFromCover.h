#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

class CombatController;

// The combat action that fires a ranged weapon from cover. It adds no fields to CombatAction.
class CombatActionAttackRangedFromCover : public CombatAction {
public:
	CombatActionAttackRangedFromCover(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the ranged weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
