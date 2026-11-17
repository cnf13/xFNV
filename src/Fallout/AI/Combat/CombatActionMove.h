#pragma once

#include "Fallout/AI/Combat/CombatAction.h"

// The combat action that moves the actor to a combat location. It adds no fields to CombatAction and overrides every
// virtual.
class CombatActionMove : public CombatAction {
public:
	CombatActionMove(CombatAction::COMBAT_ACTION aeType);

	bool CanExecute(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray,
		int32_t aiActionValue) override;
	void Execute(CombatController* apCombatController, int32_t aiActionValue) override;
	bool CheckIdentical(CombatController* apCombatController, int32_t aiActionValue) override;
	float GetCost(CombatController* apCombatController, int32_t aiActionValue) override;
	void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue) override;

	// Starts the move procedure to location aiLocation.
	static void StartMovementProcedure(CombatController* apCombatController, int32_t aiLocation);
	// Whether the controller already moves to location aiLocation.
	static bool CheckLocationIdentical(CombatController* apCombatController, int32_t aiLocation);
	// Finds a combat location of type aiLocation for the controller's combat state.
	static bool ChooseLocation(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray,
		int32_t aiLocation);
};
