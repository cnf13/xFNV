#pragma once

#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"
#include "Reconstruction.hpp"

class hkpAgent3Input;
class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpProcessCollisionOutput;

// The predictive GSK agent's data: the simplex cache and the contact manifold it keeps between steps.
struct hkPredGskAgent3Data {
	hkpGskCache m_gskCache;
	hkpGskManifold m_gskManifold;
};

// The predictive GSK stream agent between two convex shapes.
class hkPredGskAgent3 {
public:
	// Havok SDK: starts the agent's cache from the two shapes' first vertices, for a triangle shape B through
	// initTriangle, and returns the free memory after the agent's data.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// The SDK's process: collides the two convex shapes through the agent's GSK cache and manifold and writes the
	// contact points into result; returns the free memory after the agent's data.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& result);
	// Havok SDK: fills f with the predictive GSK functions; the agent is predictive.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);

	// Havok SDK: destroying the agent is its cleanup; the dispatcher is not used.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	// Havok SDK: removes the agent's contact points.
	static hkpAgentData* cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// Havok SDK: removes the manifold's point with id idToRemove, when it has one.
	static void removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
	// Havok SDK: gives the manifold's first potential point (id 0xFFFF) the id idToRemove.
	static void commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit);
	// Havok SDK: clears the dimensions of the manifold's point idToConvert.
	static void createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert);
	// Havok SDK: the separating normal of the two convex shapes from the agent's GSK cache,
	// in world space, its w the distance less both radii.
	static void sepNormal(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4& separatingNormalOut);

	// Registers the agent for the pair of shape types.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher, hkpShapeType typeA, hkpShapeType typeB);
};
