#pragma once

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTree/hkpBvTreeAgent.h"

class hkpCdPointCollector;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;
class hkpLinearCastCollisionInput;

// The bounding volume tree agent specialised for MOPP shapes.
class hkpMoppAgent : public hkpBvTreeAgent {
public:
	hkpMoppAgent(hkpContactMgr* mgr);

	// Registers the flipped agent for (MOPP, any shape), the plain one for (any shape, MOPP), then the predictive one
	// for (MOPP, MOPP).
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// The agent for two MOPP shapes, over the one with the smaller code. Havok SDK signature
	// (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createBvBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpMoppAgent.h): the linear cast against a MOPP's leaves; the registerAgent tables
	// hold it.
	static void staticLinearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpLinearCastCollisionInput& input, hkpCdPointCollector& collector, hkpCdPointCollector* startCollector);
};
