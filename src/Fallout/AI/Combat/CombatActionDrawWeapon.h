#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that draws the actor's weapon. It adds no fields to CombatAction and overrides Execute and
// GetDebugText.
class CombatActionDrawWeapon : public CombatAction {
public:
    CombatActionDrawWeapon(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
