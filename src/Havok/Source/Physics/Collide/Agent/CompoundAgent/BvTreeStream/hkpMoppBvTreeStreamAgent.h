#pragma once

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"

class hkpCollisionDispatcher;

// The registration of hkpBvTreeStreamAgent's functions for MOPP shapes. A class with no base and no object, only
// registerAgent.
class hkpMoppBvTreeStreamAgent {
public:
	// Registers the agent for MOPP shapes against convex shapes, both ways round, and MOPP
	// against MOPP.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
};
