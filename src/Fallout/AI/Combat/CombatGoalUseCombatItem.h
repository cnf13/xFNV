#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal UseCombatItem. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalUseCombatItem : public CombatGoal {
public:
    CombatGoalUseCombatItem(CombatGoal::COMBAT_GOAL aeType);
};
