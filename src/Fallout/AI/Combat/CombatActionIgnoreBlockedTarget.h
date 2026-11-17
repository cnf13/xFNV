#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that ignores a target the actor cannot reach (the PDB defines it in CombatActions.cpp). It adds
// no fields to CombatAction and overrides Execute.
class CombatActionIgnoreBlockedTarget : public CombatAction {
public:
	CombatActionIgnoreBlockedTarget(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionIgnoreBlockedTarget_vftable{};
