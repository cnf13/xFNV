#pragma once

#include "Reconstruction.hpp"

class hkpEntity;

// The interface the engine reports entity events to, each callback taking the hkpEntity that changed. The interface
// itself holds no state; listeners that carry state live in the classes deriving from it (bhkEntityListener and
// friends).
class hkpEntityListener {
public:
	hkpEntityListener();
	virtual ~hkpEntityListener();

	virtual void entityAddedCallback(hkpEntity*) {}
	virtual void entityRemovedCallback(hkpEntity*) {}
	virtual void entityShapeSetCallback(hkpEntity* entity);
	virtual void entitySetMotionTypeCallback(hkpEntity*) {}
	virtual void entityDeletedCallback(hkpEntity*) {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpEntityListener_vftable{};
