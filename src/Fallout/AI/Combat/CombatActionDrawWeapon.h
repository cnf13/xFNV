#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that draws the actor's weapon (the PDB defines it in CombatActions.cpp). It adds no fields to
// CombatAction and overrides Execute and GetDebugText (IDA names the latter GetName).
class CombatActionDrawWeapon : public CombatAction {
public:
	CombatActionDrawWeapon(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionDrawWeapon_vftable{};
