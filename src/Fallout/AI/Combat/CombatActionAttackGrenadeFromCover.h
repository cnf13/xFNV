#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// A grenade attack from cover; the constructor adds no fields.
class CombatActionAttackGrenadeFromCover : public CombatAction {
public:
	CombatActionAttackGrenadeFromCover(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackGrenadeFromCover_vftable{};
