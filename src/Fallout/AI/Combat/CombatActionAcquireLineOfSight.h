#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that gets a 360-degree line of sight to the target. It adds no fields to CombatAction and overrides
// no virtual.
class CombatActionAcquireLineOfSight : public CombatAction {
public:
    CombatActionAcquireLineOfSight(CombatAction::COMBAT_ACTION aeType);
};
