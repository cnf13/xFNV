#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

class CombatController;

// The combat action that fires an explosive ranged weapon at the target (the PDB defines it in CombatActions.cpp). It
// adds no fields to CombatAction.
class CombatActionAttackRangedExplosive : public CombatAction {
public:
	CombatActionAttackRangedExplosive(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the explosive ranged weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackRangedExplosive_vftable{};
