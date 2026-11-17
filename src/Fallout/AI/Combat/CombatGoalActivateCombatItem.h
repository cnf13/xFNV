#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal ActivateCombatItem. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalActivateCombatItem : public CombatGoal {
public:
	CombatGoalActivateCombatItem(CombatGoal::COMBAT_GOAL aeType);
};
