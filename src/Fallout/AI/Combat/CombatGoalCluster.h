#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"

// The combat planner goal Cluster. It adds no fields to CombatGoal and overrides no virtual.
class CombatGoalCluster : public CombatGoal {
public:
	CombatGoalCluster(CombatGoal::COMBAT_GOAL aeType);
};
