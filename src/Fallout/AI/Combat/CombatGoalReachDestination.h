#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal ReachDestination. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalReachDestination : public CombatGoal {
public:
	CombatGoalReachDestination(CombatGoal::COMBAT_GOAL aeType);
};
