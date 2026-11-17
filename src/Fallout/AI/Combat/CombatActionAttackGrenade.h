#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

class CombatController;

// The combat action that throws a grenade at the target (the PDB defines it in CombatActions.cpp). It adds no fields
// to CombatAction. CombatActionAttackGrenadeFromCover's GetCost is the same function (0x97BB90), registered here.
class CombatActionAttackGrenade : public CombatAction {
public:
	CombatActionAttackGrenade(CombatAction::COMBAT_ACTION aeType);

	// Queues the grenade attack procedure on the controller.
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	// The base cost plus the grenade weapon cost, rounded to float.
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackGrenade_vftable{};
