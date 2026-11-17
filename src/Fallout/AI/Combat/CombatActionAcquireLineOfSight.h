#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that gets a 360-degree line of sight to the target (the PDB defines it in CombatActions.cpp). It
// adds no fields to CombatAction and overrides no virtual (its exe vtable holds CombatAction's entries).
class CombatActionAcquireLineOfSight : public CombatAction {
public:
	CombatActionAcquireLineOfSight(CombatAction::COMBAT_ACTION aeType);
};
inline constexpr fnvd::Vtable CombatActionAcquireLineOfSight_vftable{};
