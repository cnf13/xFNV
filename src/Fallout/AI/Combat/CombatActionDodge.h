#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that dodges (the PDB defines it in CombatActions.cpp). It adds no fields to CombatAction and
// overrides no virtual (its exe vtable holds CombatAction's entries).
class CombatActionDodge : public CombatAction {
public:
	CombatActionDodge(CombatAction::COMBAT_ACTION aeType);
};
inline constexpr fnvd::Vtable CombatActionDodge_vftable{};
