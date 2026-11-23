#pragma once

#include "Fallout/AI/Combat/CombatActionMove.h"

// Moves while switching to a melee weapon; the constructor adds no fields.
class CombatActionMoveAndSwitchToMelee : public CombatActionMove {
public:
    CombatActionMoveAndSwitchToMelee(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
