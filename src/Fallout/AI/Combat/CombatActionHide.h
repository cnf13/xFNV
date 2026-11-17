#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// Hides at the selected combat location; the constructor adds no fields.
class CombatActionHide : public CombatAction {
public:
	CombatActionHide(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
