#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

class CombatController;

// The combat action that attacks with the fists (the PDB defines it in CombatActions.cpp). Only the entry points the
// reconstructed code defines are declared.
class CombatActionAttackHandToHand : public CombatAction {
public:
	CombatActionAttackHandToHand(CombatAction::COMBAT_ACTION aeType);

	// Same function as CombatActionAttackMelee::Execute (thiscall, ret 8, 0x97B410).
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the hand-to-hand cost; out of reach for a player teammate holding a weapon.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackHandToHand_vftable{};
