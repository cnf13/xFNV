#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that moves the actor away from a threat. It adds no fields to CombatAction and overrides no
// virtual.
class CombatActionAvoidThreat : public CombatAction {
public:
	CombatActionAvoidThreat(CombatAction::COMBAT_ACTION aeType);
};
