#pragma once

#include "BSHavok/bhkEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"

// An entity listener that also hears about phantoms: the Havok side of a trap trigger.
class bhkTrapListener : public bhkEntityListener, public hkpPhantomListener {
public:
	bhkTrapListener();
	~bhkTrapListener() override;

	// DeletingDestruct's free (ourFree); both bases declare one.
	static void operator delete(void* apMemory) { ourFree(apMemory); }
};
