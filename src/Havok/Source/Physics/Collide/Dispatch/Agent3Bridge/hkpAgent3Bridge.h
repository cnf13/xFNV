#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Reconstruction.hpp"

class hkpCollisionAgent;
class hkpCollisionDispatcher;
class hkpConstraintOwner;
class hkpContactMgr;

// The stream agent that wraps a classic (agent2) collision agent. src models only its registration; the agent itself
// stays with the exe.
class hkAgent3Bridge {
public:
	// Registers the bridge for every pair of shape types.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// The PDB's destroy, the bridge's hkAgent3::DestroyFunc: calls the wrapped agent's cleanup virtual (the agent
	// pointer the agent data holds, slot 7) with constraintOwner (cdecl, ret 0, 0xD921E0).
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	// The bridge's hkAgent3::RemovePointFunc: the wrapped agent's removePoint virtual (slot 11).
	static void removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
	// The bridge's hkAgent3::CreateFunc: creates the classic agent for the pair's shape types into the agent data
	// and marks the entry as a call into a classic agent.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// The bridge's hkAgent3::ProcessFunc: the wrapped agent's processCollision virtual (slot 6).
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormalOut, hkpProcessCollisionOutput& result);

	// The PDB's helpers, inlined by the exe: the agent data is untyped stream memory whose first bytes hold the
	// wrapped agent's pointer, and the bridge's data takes 16 bytes of the stream.
	static hkpCollisionAgent*& getChildAgent(hkpAgentData* agentData) {
		return *static_cast<hkpCollisionAgent**>(agentData);
	}

	static hkpAgentData* getAgentDataEnd(hkpAgentData* agentData) { return static_cast<uint8_t*>(agentData) + 16; }
};
