#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal FindTarget (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and
// overrides no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalFindTarget : public CombatGoal {
public:
	CombatGoalFindTarget(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalFindTarget_vftable{};
