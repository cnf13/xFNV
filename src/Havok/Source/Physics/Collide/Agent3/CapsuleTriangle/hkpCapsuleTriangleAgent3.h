#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Util/hkpCollideTriangleUtil.h"
#include "Reconstruction.hpp"

class hkpAgent3Input;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpCollisionDispatcher;
class hkpProcessCollisionOutput;

// The stream agent's data: the ids of its three contact points (hkContactPointId, an unsigned short; 0xFFFF for
// none) and the triangle query's cache. The agent's free memory starts at the next multiple of 16.
struct hkpCapsuleTriangleCache3 {
	uint16_t m_contactPointId[3];
	hkpCollideTriangleUtil::PointTriangleDistanceCache m_triangleCache;
};

// The stream agent between a capsule and a triangle.
class hkCapsuleTriangleAgent3 {
public:
	// No contact points, the three point ids free, the triangle's distance cache set up from body B's triangle;
	// returns the agent's free memory after its data.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// Removes the agent's valid contact points through mgr and frees their ids; returns the agent's free memory
	// after its data.
	static hkpAgentData* cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// cleanup, the dispatcher unused.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	// The separating normal of the nearer of the utility's first two points for the capsule A and the triangle B in
	// world space.
	static void sepNormal(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4& separatingNormalOut);
	// The capsule A's end points and the triangle B's vertices in world space, the utility's three candidate points
	// and the input's tolerance decide the agent's contact points.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& result);
	// Registers the agent for a capsule against a triangle.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
};
