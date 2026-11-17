#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

// The character state for flying. Only its vtable is declared: bhkCharacterControllerCinfo's constructor builds it
// inline.
class bhkCharacterStateFlying : public bhkCharacterState {
public:
	// The flying state type (slot 3).
	hkpCharacterStateType getType() const override { return HK_CHARACTER_FLYING; }
	// Flying does nothing per update (slot 8).
	void Update(bhkCharacterController& arController) override {}
};

inline constexpr fnvd::Vtable bhkCharacterStateFlying_vftable{};
