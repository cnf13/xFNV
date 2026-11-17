#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

class CombatController;

// The combat action that throws a grenade at the target. It adds no fields to CombatAction.
class CombatActionAttackGrenade : public CombatAction {
public:
	CombatActionAttackGrenade(CombatAction::COMBAT_ACTION aeType);

	// Queues the grenade attack procedure on the controller.
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the grenade weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
