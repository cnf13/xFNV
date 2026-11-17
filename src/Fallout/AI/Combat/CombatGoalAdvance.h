#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Advance. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalAdvance : public CombatGoal {
public:
	CombatGoalAdvance(CombatGoal::COMBAT_GOAL aeType);
};
