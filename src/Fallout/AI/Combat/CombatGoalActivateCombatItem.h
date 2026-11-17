#pragma once

#include "Fallout/AI/Combat/CombatGoal.h"
#include "Reconstruction.hpp"

// The combat planner goal ActivateCombatItem (the PDB defines it in CombatGoal.cpp). It adds no fields to CombatGoal
// and overrides no virtual (its exe vtable holds CombatGoal's entries).
class CombatGoalActivateCombatItem : public CombatGoal {
public:
	CombatGoalActivateCombatItem(CombatGoal::COMBAT_GOAL aeType);
};
inline constexpr fnvd::Vtable CombatGoalActivateCombatItem_vftable{};
