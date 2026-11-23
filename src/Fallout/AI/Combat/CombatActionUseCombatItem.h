#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// Uses a combat item; the constructor changes only the base action's state lists and vtable.
class CombatActionUseCombatItem : public CombatAction {
public:
    CombatActionUseCombatItem(CombatAction::COMBAT_ACTION aeType);

    void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;
};
