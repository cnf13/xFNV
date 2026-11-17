#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// An explosive ranged attack from cover; the constructor adds no fields.
class CombatActionAttackRangedExplosiveFromCover : public CombatAction {
public:
	CombatActionAttackRangedExplosiveFromCover(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionAttackRangedExplosiveFromCover_vftable{};
