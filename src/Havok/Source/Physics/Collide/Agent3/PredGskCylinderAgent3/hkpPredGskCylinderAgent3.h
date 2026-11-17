#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"

struct hkpAgentEntry;
class hkpCdBody;
class hkpCollisionDispatcher;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpGskCache;
class hkpGskManifold;

// File-local helper of the cylinder agent's process. For each of the two bodies whose isCylinder entry is set: builds a
// capsule on the cylinder's two vertices (radius: its cylinder radius plus its convex radius plus 0.002) in
// temporaryCdBodies[i], with the body's motion and the body as parent, keeps the body in originalCdBodies[i] and points
// *cdBodies[i] at the temporary body; else clears originalCdBodies[i].
void replaceCylindersWithCapsulesInCdBodies(const hkpCdBody** cdBodies[2], hkpCdBody* isReplacedWithCapsule,
	const int32_t* isCylinder, const hkpCdBody** originalCdBodies);
// File-local helper of the cylinder agent's process.
float separatingDistanceUsingCapsuleRepresentations(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry,
	void* agentData);
void restoreCdBodies(const hkpCdBody* const* originalCdBodies, const hkpCdBody** const* cdBodies);
// Tail of an agent's cleanup: hkGskManifold_cleanup on the manifold with the contact manager and the constraint owner,
// then the entry's m_numContactPoints set to the manifold's. Returns the entry.
hkpAgentEntry* cleanupManifold(hkpAgentEntry* entry, hkpGskManifold& manifold, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner);
// Converts a GSK cache's vertex ids of the bodies a cylinder agent replaces by capsules.
void convertCapsuleCache(const int32_t* isReplacedWithCapsule, hkpGskCache* cache);

// The predictive GSK stream agent between a cylinder and a convex shape: hkPredGskAgent3's functions, with each
// cylinder of the pair stood in for by a capsule while the agent's GSK cache says so.
class hkPredGskCylinderAgent3 {
public:
	// Registers the agent for a cylinder against a convex shape and the other way.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// Havok SDK: hkPredGskAgent3::create, then the GSK cache's cylinder flags set for each body whose shape is a
	// cylinder; returns the free memory after the agent's data.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// Havok SDK: hkPredGskAgent3's process with each cylinder of the pair stood in for by a capsule while its cache
	// says so (replaceCylindersWithCapsulesInCdBodies); returns the free memory after the agent's data.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& result);
};
