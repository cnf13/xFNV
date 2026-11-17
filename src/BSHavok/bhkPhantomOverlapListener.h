#pragma once

#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Reconstruction.hpp"

// The game's phantom overlap listener, a singleton held in pListener. KillInstance deletes it through slot 2, the
// base's deleting destructor. Its own callbacks and constructor are not modelled yet (the exe has no RTTI for it).
class bhkPhantomOverlapListener : public hkpPhantomOverlapListener {
public:
	// Deletes the singleton (its scalar deleting destructor, flags 1) inside the Havok heap context; pListener itself
	// is left as it is.
	static void KillInstance();

	static inline bhkPhantomOverlapListener* pListener;
};
