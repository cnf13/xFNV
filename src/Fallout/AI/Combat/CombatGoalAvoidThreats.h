#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal AvoidThreats. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalAvoidThreats : public CombatGoal {
public:
	CombatGoalAvoidThreats(CombatGoal::COMBAT_GOAL aeType);
};
