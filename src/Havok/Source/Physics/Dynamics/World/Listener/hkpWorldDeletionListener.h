#pragma once

#include "Reconstruction.hpp"

class hkpWorld;

// The interface told when a world is deleted or emptied. It holds no state.
class hkpWorldDeletionListener {
public:
	virtual ~hkpWorldDeletionListener();

	virtual void worldDeletedCallback(hkpWorld* world) = 0;
	virtual void worldRemoveAllCallback(hkpWorld*) {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};

inline constexpr fnvd::Vtable hkpWorldDeletionListener_vftable{};
