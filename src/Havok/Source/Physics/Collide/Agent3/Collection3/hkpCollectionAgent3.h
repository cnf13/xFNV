#pragma once

#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Reconstruction.hpp"

class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpProcessCollisionOutput;

// The SDK's hkpAgentData is void: the bytes of an agent's data, in front of its entry.
typedef void hkpAgentData;

// The stream agent between a convex shape and a shape collection: it wraps the single child the body's shape names
// in a one-element list shape and hands the pair to the collection-collection agent.
class hkpCollectionAgent3 {
public:
	// Registers the agent for a convex shape against a shape collection.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// The SDK's initAgentFunc: hkListAgent3's functions, with this agent's process.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);
	// The agent's process: the body's shape as a one-child list, processed against the other collection.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& output);
};
