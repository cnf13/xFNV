#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal Guard (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal and overrides
// no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalGuard : public CombatGoal {
public:
	CombatGoalGuard(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalGuard_vftable{};
