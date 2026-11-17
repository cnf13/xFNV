#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that moves the actor toward its target (the PDB defines it in CombatActions.cpp). It adds no fields
// to CombatAction and overrides no virtual (its exe vtable holds CombatAction's entries).
class CombatActionApproachTarget : public CombatAction {
public:
	CombatActionApproachTarget(CombatAction::COMBAT_ACTION aeType);
};
inline constexpr fnvd::Vtable CombatActionApproachTarget_vftable{};
