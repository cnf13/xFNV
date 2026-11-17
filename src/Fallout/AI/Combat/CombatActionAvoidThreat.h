#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that moves the actor away from a threat (the PDB defines it in CombatActions.cpp). It adds no
// fields to CombatAction and overrides no virtual (its exe vtable holds CombatAction's entries).
class CombatActionAvoidThreat : public CombatAction {
public:
	CombatActionAvoidThreat(CombatAction::COMBAT_ACTION aeType);
};
inline constexpr fnvd::Vtable CombatActionAvoidThreat_vftable{};
