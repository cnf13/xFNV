#pragma once

#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"

#include <stdint.h>

class hkpCdBody;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpContactMgr;

// Sorts the keys between low and high in place.
void quickSortRecursive(uint32_t* keys, int32_t low, int32_t high, int32_t unused);

// The agent between a bounding volume tree shape and another shape that keeps its child agents in a stream (an agent
// track) and caches the queried AABB.
class hkpBvTreeStreamAgent : public hkpCollisionAgent {
public:
	hkpBvTreeStreamAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpContactMgr* mgr);
	~hkpBvTreeStreamAgent() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getPenetrations(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdBodyPairCollector& collector) override;
	void getClosestPoints(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input,
		hkpCdPointCollector& collector) override;
	void linearCast(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpLinearCastCollisionInput& input,
		hkpCdPointCollector& collector, hkpCdPointCollector* startCollector) override;
	void processCollision(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpProcessCollisionInput& input,
		hkpProcessCollisionOutput& result) override;
	void cleanup(hkpConstraintOwner& info) override;
	void updateShapeCollectionFilter(const hkpCdBody& bodyA, const hkpCdBody& collectionBodyB,
		const hkpCollisionInput& input, hkpConstraintOwner& constraintOwner) override;
	void invalidateTim(const hkpCollisionInput& input) override;
	void warpTime(float oldTime, float newTime, const hkpCollisionInput& input) override;
	void removePoint(uint16_t idToRemove) override {}
	void commitPotential(uint16_t newId) override {}
	void createZombie(uint16_t idToBecomeZombie) override {}

	// Registers the agent for bounding volume trees against convex shapes, both ways round.
	static void registerAgent(hkpCollisionDispatcher* dispatcher);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createShapeBvAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by bytes).
	static hkpCollisionAgent* dispatchConvexListBvTree(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static hkpCollisionAgent* createBvTreeShapeAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Havok SDK signature (hkpCollide.lib, tied by bytes).
	static hkpCollisionAgent* dispatchBvTreeConvexList(const hkpCdBody& bodyA, const hkpCdBody& bodyB,
		const hkpCollisionInput& input, hkpContactMgr* mgr);
	// Registers the agent for bounding volume trees against multi-ray shapes, both ways round.
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	static void registerMultiRayAgent(hkpCollisionDispatcher* dispatcher);
	// Registers the agent for bounding volume trees against convex lists, both ways round.
	static void registerConvexListAgent(hkpCollisionDispatcher* dispatcher);

	hkpCollisionDispatcher* m_dispatcher;
	hkAabb m_cachedAabb;
	// The child agents' stream.
	hkpAgent1nTrack m_agentTrack;
	uint8_t padding3C[4];
};
