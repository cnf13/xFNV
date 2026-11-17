#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that switches the actor's weapon. It adds no fields to CombatAction.
class CombatActionSwitchWeapon : public CombatAction {
public:
    CombatActionSwitchWeapon(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
    float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
