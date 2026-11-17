#pragma once

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Reconstruction.hpp"

class hkpCollisionDispatcher;

// The registration of hkpBvTreeStreamAgent's functions for MOPP shapes. As in the SDK, a class with no base and no
// object: the exe has no vtable for it, only registerAgent.
class hkpMoppBvTreeStreamAgent {
public:
	// Registers the agent for MOPP shapes against convex shapes, both ways round, and MOPP
	// against MOPP.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
};
