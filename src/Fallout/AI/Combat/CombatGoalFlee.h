#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal Flee (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and overrides no
// virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalFlee : public CombatGoal {
public:
	CombatGoalFlee(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalFlee_vftable{};
