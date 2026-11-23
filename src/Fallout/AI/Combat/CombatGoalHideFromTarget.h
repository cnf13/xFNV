#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal HideFromTarget. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalHideFromTarget : public CombatGoal {
public:
    CombatGoalHideFromTarget(CombatGoal::COMBAT_GOAL aeType);
};
