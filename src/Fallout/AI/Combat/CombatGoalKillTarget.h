#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal KillTarget. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalKillTarget : public CombatGoal {
public:
	CombatGoalKillTarget(CombatGoal::COMBAT_GOAL aeType);
};
