#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"

class hkpCollisionAgent;
class hkpCollisionDispatcher;
class hkpConstraintOwner;
class hkpContactMgr;

// The stream agent that wraps a classic (agent2) collision agent.
class hkAgent3Bridge {
public:
	// Registers the bridge for every pair of shape types.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// The bridge's hkAgent3::DestroyFunc: calls the wrapped agent's cleanup virtual (the agent pointer the agent data
	// holds) with constraintOwner.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	// The bridge's hkAgent3::RemovePointFunc: calls the wrapped agent's removePoint virtual.
	static void removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
	// The bridge's hkAgent3::CreateFunc: creates the classic agent for the pair's shape types into the agent data
	// and marks the entry as a call into a classic agent.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// The bridge's hkAgent3::ProcessFunc: calls the wrapped agent's processCollision virtual.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormalOut, hkpProcessCollisionOutput& result);

	// The agent data is untyped stream memory whose first bytes hold the wrapped agent's pointer, and the bridge's data
	// takes 16 bytes of the stream.
	static hkpCollisionAgent*& getChildAgent(hkpAgentData* agentData) {
		return *static_cast<hkpCollisionAgent**>(agentData);
	}

	static hkpAgentData* getAgentDataEnd(hkpAgentData* agentData) { return static_cast<uint8_t*>(agentData) + 16; }
};
