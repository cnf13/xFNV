#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal StayInCombatArea. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalStayInCombatArea : public CombatGoal {
public:
	CombatGoalStayInCombatArea(CombatGoal::COMBAT_GOAL aeType);
};
