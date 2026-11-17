#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that dodges. It adds no fields to CombatAction and overrides no virtual.
class CombatActionDodge : public CombatAction {
public:
	CombatActionDodge(CombatAction::COMBAT_ACTION aeType);
};
