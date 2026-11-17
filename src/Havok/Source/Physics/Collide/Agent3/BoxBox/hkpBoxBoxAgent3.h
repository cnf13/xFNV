#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpProcessCollisionOutput;

// The stream agent between two boxes: a box-box manifold in the agent's data, filled by the box-box detector.
class hkBoxBoxAgent3 {
public:
	// The agent's process: the box-box collision detection over the pair's bodies, adding its contact points to
	// output.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& output);

	// Removes the agent's contact points from the constraint owner; returns the end of the agent's data.
	static hkpAgentData* cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);

	// Destroys the agent: cleanup does it all.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);

	// The SDK's placement new of the manifold (with its null test); returns the memory past it.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// Removes the contact point idToRemove from the manifold.
	static void removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
	// Gives the first potential point of the manifold the id newId.
	static void commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit);
	// Empty.
	static void createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert);
	// Fills f with the agent's functions.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);

	// Registers the agent for a box against a box.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
};
