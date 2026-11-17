#pragma once

#include "Reconstruction.hpp"

class hkpPhantom;

// Callbacks a phantom fires on its listeners, each taking the phantom that changed.
class hkpPhantomListener {
public:
	hkpPhantomListener();
	virtual ~hkpPhantomListener();

	virtual void phantomAddedCallback(hkpPhantom*) {}
	virtual void phantomRemovedCallback(hkpPhantom*) {}
	// folded with hkpEntityListener::entityShapeSetCallback
	virtual void phantomShapeSetCallback(hkpPhantom* phantom);
	virtual void phantomDeletedCallback(hkpPhantom*) {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpPhantomListener_vftable{};
