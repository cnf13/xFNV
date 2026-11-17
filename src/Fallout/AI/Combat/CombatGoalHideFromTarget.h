#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal HideFromTarget (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and
// overrides no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalHideFromTarget : public CombatGoal {
public:
	CombatGoalHideFromTarget(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalHideFromTarget_vftable{};
