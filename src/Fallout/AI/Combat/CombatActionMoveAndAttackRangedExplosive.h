#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatActionAttackRangedExplosive.h"

// An explosive ranged attack performed while moving; the constructor adds no fields.
class CombatActionMoveAndAttackRangedExplosive : public CombatActionAttackRangedExplosive {
public:
	CombatActionMoveAndAttackRangedExplosive(CombatAction::COMBAT_ACTION aeType);

	bool CanExecute(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray,
		int32_t aiActionValue) override;
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	bool CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
