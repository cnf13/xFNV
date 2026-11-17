#pragma once

#include "Reconstruction.hpp"

class CombatState;

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
// Inferred type: apThis: CombatState* from 0x97b8e2 (CombatController::pCombatState (parameter 0 of
// CombatActionAttackGrenadeFlushTarget::Execute)), its members used at 0x97b96a
bool GetGrenadeThrowHighArc(CombatState* apThis, [[maybe_unused]] void* apEdx);
