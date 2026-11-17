#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal StayInCombatArea (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and
// overrides no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalStayInCombatArea : public CombatGoal {
public:
	CombatGoalStayInCombatArea(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalStayInCombatArea_vftable{};
