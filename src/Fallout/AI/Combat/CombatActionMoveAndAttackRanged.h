#pragma once

#include "Fallout/AI/Combat/CombatActionAttackRanged.h"

// A ranged attack performed while moving; its constructor adds no fields to the ranged action.
class CombatActionMoveAndAttackRanged : public CombatActionAttackRanged {
public:
    CombatActionMoveAndAttackRanged(CombatAction::COMBAT_ACTION aeType);

    bool CanExecute(
        CombatController* apCombatController,
        CombatWorldStateArray* apWorldStateArray,
        int32_t aiActionValue
    ) override;
    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
    bool CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) override;
    float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
