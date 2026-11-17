#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Flank. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalFlank : public CombatGoal {
public:
    CombatGoalFlank(CombatGoal::COMBAT_GOAL aeType);
};
