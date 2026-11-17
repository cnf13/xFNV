#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkAabb;
class hkStatisticsCollector;
class hkTransform;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpCdBody;
class hkpCollisionInput;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpShapeCollection;
class hkpShapeContainer;

// The stream agent between any shape and a bounding volume tree.
class hkBvTreeAgent3 {
public:
	// The agent's process: the shape against the bounding volume tree, adding its contact points to output.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& output);
	// Registers the agent for any shape against a bounding volume tree.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// Fills f with the agent's functions and makes it predictive.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);
	// The agent3 table's filter update: re-filters the child agents of the track agentData holds against B's
	// container.
	static void updateFilter(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCdBody& bodyA,
		const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// The agent3 table's statistics of the track agentData holds.
	static void calcStatistics(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// The agent3 table's time of impact reset of the track agentData holds.
	static void invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
	// The agent3 table's warp time: the entry is not used, the agent data is the track.
	static void warpTime(hkpAgentEntry* entry, hkpAgentData* agentData, float oldTime, float newTime,
		const hkpCollisionInput& input);
	// The AABB of A in B's space, grown by the motion over the step, queried against B's tree into hitListOut
	// (sorted, terminated by the invalid key): the number of hits, or -1 when cachedAabb already holds the AABB.
	static int32_t calcAabbAndQueryTree(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkTransform& bTa,
		const hkVector4& linearTimInfo, const hkpProcessCollisionInput& input, hkAabb* cachedAabb, uint32_t* hitListOut,
		int32_t hitListCapacity);
	// The shape container of body's bounding volume tree shape; buffer (the SDK's hkpShapeBuffer, 512 bytes, 16-byte
	// aligned) is for a container built on the stack.
	static const hkpShapeContainer* getShapeContainerFrom(const hkpCdBody* body, char* buffer);
	// The child of body's MOPP or triangle sampled heightfield tree, or null; buffer (the SDK's hkpShapeBuffer) is
	// not used.
	static const hkpShapeCollection* getShapeCollectionIfBvTreeSupportsAabbQueries(const hkpCdBody* body, char* buffer);
};
