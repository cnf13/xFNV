#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that investigates the target's last known location (the PDB defines it in CombatActions.cpp). It
// adds no fields to CombatAction and overrides no virtual (its exe vtable holds CombatAction's entries).
class CombatActionInvestigate : public CombatAction {
public:
	CombatActionInvestigate(CombatAction::COMBAT_ACTION aeType);
};
inline constexpr fnvd::Vtable CombatActionInvestigate_vftable{};
