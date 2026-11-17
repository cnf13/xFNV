#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal AvoidThreats (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and
// overrides no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalAvoidThreats : public CombatGoal {
public:
	CombatGoalAvoidThreats(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalAvoidThreats_vftable{};
