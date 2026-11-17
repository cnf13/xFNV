#pragma once

#include "Fallout/AI/Combat/CombatAction.h"
#include "Reconstruction.hpp"

// The combat action that searches for a lost target (the PDB defines it in CombatActions.cpp). It adds no fields to
// CombatAction and overrides Execute (IDA names it AddProcedureToController).
class CombatActionSearch : public CombatAction {
public:
	CombatActionSearch(CombatAction::COMBAT_ACTION aeType);

	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
};
inline constexpr fnvd::Vtable CombatActionSearch_vftable{};
