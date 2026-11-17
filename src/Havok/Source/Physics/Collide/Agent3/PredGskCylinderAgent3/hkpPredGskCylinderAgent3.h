#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Reconstruction.hpp"

struct hkpAgentEntry;
class hkpCdBody;
class hkpCollisionDispatcher;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpGskCache;
class hkpGskManifold;

// The PDB's file-local helper of the cylinder agent's process. For each of the two bodies
// whose isCylinder entry is set: builds a capsule on the cylinder's two vertices (radius: its cylinder radius plus
// its convex radius plus 0.002) in temporaryCdBodies[i], with the body's motion and the body as parent, keeps the
// body in originalCdBodies[i] and points *cdBodies[i] at the temporary body; else clears originalCdBodies[i]
// (usercall: isCylinder in eax, originalCdBodies in ecx, cdBodies and temporaryCdBodies on the stack; ret 0,
// 0xD40A10).
void replaceCylindersWithCapsulesInCdBodies(const hkpCdBody** cdBodies[2], hkpCdBody* isReplacedWithCapsule,
	const int32_t* isCylinder, const hkpCdBody** originalCdBodies);
// Havok SDK signature (hkpCollide.lib, tied by bytes); the PDB's file-local helper of the cylinder agent's process
// (cdecl, ret 0, 0xD40BE0).
float separatingDistanceUsingCapsuleRepresentations(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry,
	void* agentData);
// Not named in the PDB or IDA; name chosen here. usercall: originalCdBodies in ecx, cdBodies in eax.
void restoreCdBodies(const hkpCdBody* const* originalCdBodies, const hkpCdBody** const* cdBodies);
// Not named in the PDB or IDA; name chosen here. A compiler-split tail of an agent's cleanup:
// hkGskManifold_cleanup on the manifold in esi with the contact manager in ecx and the constraint owner in eax (as
// the caller left them), then the entry's m_numContactPoints (+0x2) set to the manifold's (+0x2); eax keeps the entry
// (usercall: entry on the stack, ret 0, 0xD409B0).
hkpAgentEntry* cleanupManifold(hkpAgentEntry* entry, hkpGskManifold& manifold, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner);
// Not named in the PDB or IDA; name chosen here. Converts a GSK cache's vertex ids of the bodies a cylinder agent
// replaces by capsules (usercall: isReplacedWithCapsule in edx, cache in eax, ret 0, 0xD40AF0).
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
