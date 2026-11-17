#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Ambush. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalAmbush : public CombatGoal {
public:
	CombatGoalAmbush(CombatGoal::COMBAT_GOAL aeType);
};
