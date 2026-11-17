#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal FindTarget. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalFindTarget : public CombatGoal {
public:
    CombatGoalFindTarget(CombatGoal::COMBAT_GOAL aeType);
};
