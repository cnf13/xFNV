#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that moves the actor toward its target. It adds no fields to CombatAction and overrides no virtual.
class CombatActionApproachTarget : public CombatAction {
public:
    CombatActionApproachTarget(CombatAction::COMBAT_ACTION aeType);
};
