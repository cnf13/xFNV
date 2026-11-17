#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that searches for a lost target. It adds no fields to CombatAction and overrides Execute.
class CombatActionSearch : public CombatAction {
public:
    CombatActionSearch(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
