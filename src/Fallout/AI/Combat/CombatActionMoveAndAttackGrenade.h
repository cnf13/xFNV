#pragma once

#include "Fallout/AI/Combat/CombatActionAttackGrenade.h"

// A grenade attack performed while moving; the constructor adds no fields.
class CombatActionMoveAndAttackGrenade : public CombatActionAttackGrenade {
public:
	CombatActionMoveAndAttackGrenade(CombatAction::COMBAT_ACTION aeType);

	bool CanExecute(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray,
		int32_t aiActionValue) override;
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	bool CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
