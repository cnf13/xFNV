#pragma once

#include "BSHavok/bhkCharacterState.h"
#include "Reconstruction.hpp"

// The character state used for projectile movement.
class bhkCharacterStateProjectile : public bhkCharacterState {
public:
	// The constructor, run on apMemory.
	bhkCharacterStateProjectile();

	hkpCharacterStateType getType() const override;
	void Update(bhkCharacterController& arInput) override;
};
inline constexpr fnvd::Vtable bhkCharacterStateProjectile_vftable{};
