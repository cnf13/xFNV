#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

class CombatController;

// The combat action that attacks with the fists. Only some of its entry points are declared.
class CombatActionAttackHandToHand : public CombatAction {
public:
	CombatActionAttackHandToHand(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the hand-to-hand cost; out of reach for a player teammate holding a weapon.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
