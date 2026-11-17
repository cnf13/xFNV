#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal FallBack. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalFallBack : public CombatGoal {
public:
	CombatGoalFallBack(CombatGoal::COMBAT_GOAL aeType);
};
