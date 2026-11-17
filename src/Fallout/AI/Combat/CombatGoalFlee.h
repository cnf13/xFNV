#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Flee. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalFlee : public CombatGoal {
public:
	CombatGoalFlee(CombatGoal::COMBAT_GOAL aeType);
};
