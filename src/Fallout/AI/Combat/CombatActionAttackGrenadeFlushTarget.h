#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that throws a grenade to flush the target out of cover (the PDB defines it in
// CombatActions.cpp). It adds no fields to CombatAction.
class CombatActionAttackGrenadeFlushTarget : public CombatAction {
public:
	CombatActionAttackGrenadeFlushTarget(CombatAction::COMBAT_ACTION aeType);

	bool CanExecute(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray,
		int32_t aiActionValue) override;
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackGrenadeFlushTarget_vftable{};
