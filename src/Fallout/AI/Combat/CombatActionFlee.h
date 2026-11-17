#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// Flees to a cover location; the constructor adds no fields.
class CombatActionFlee : public CombatAction {
public:
	CombatActionFlee(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionFlee_vftable{};
