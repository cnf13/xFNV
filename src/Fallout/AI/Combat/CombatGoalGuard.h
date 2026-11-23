#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Guard. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalGuard : public CombatGoal {
public:
    CombatGoalGuard(CombatGoal::COMBAT_GOAL aeType);
};
