#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that investigates the target's last known location. It adds no fields to CombatAction and overrides
// no virtual.
class CombatActionInvestigate : public CombatAction {
public:
    CombatActionInvestigate(CombatAction::COMBAT_ACTION aeType);
};
