#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal FlushTarget. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalFlushTarget : public CombatGoal {
public:
	CombatGoalFlushTarget(CombatGoal::COMBAT_GOAL aeType);
};
