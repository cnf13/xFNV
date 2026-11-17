#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that ignores a target the actor cannot reach. It adds no fields to CombatAction and overrides
// Execute.
class CombatActionIgnoreBlockedTarget : public CombatAction {
public:
    CombatActionIgnoreBlockedTarget(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
