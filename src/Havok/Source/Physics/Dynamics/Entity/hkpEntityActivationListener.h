#pragma once

#include "Reconstruction.hpp"

class hkpEntity;

// The interface the engine reports an entity's deactivation and reactivation to. It holds no state.
class hkpEntityActivationListener {
public:
	hkpEntityActivationListener();
	virtual ~hkpEntityActivationListener();

	virtual void entityDeactivatedCallback(hkpEntity* apEntity) = 0;
	virtual void entityActivatedCallback(hkpEntity* apEntity) = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpEntityActivationListener_vftable{};
