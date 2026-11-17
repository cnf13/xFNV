#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

class CombatController;

// The combat action that attacks with a melee weapon. Only some of its entry points are declared.
class CombatActionAttackMelee : public CombatAction {
public:
	CombatActionAttackMelee(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the melee weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
