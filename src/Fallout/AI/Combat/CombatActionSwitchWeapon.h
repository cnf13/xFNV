#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that switches the actor's weapon (the PDB defines it in CombatActions.cpp). It adds no fields to
// CombatAction. Its Execute and GetDebugText entries are shared with CombatActionDrawWeapon in the exe (IDA names the
// latter GetName).
class CombatActionSwitchWeapon : public CombatAction {
public:
	CombatActionSwitchWeapon(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionSwitchWeapon_vftable{};
