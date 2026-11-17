#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

class hkpAgent3ProcessInput;
struct hkpAgentEntry;
class hkpCollisionInput;
class hkpProcessCollisionOutput;
class hkStatisticsCollector;

// The stream agent that collides a shape against each child of a list shape. It is a class of statics, following the
// other stream agents (hkBoxBoxAgent3).
class hkListAgent3 {
public:
	// Registers the agent's functions with dispatcher for the list shape.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);
	// The SDK's initAgentFunc: fills f with the list agent's functions.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);
	// The SDK's process: every child of the list shape collides against the other shape, through the child agents
	// the entry names; returns the free memory after the agent's data.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& result);
	static void calcStatistics(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// Creates the agent's track in freeMemory; returns the memory after it.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// Destroys the track agentData holds; the entry is not read.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	// Re-filters the child agents of the track agentData holds against B's container.
	static void updateFilter(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCdBody& bodyA,
		const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// The time of impact reset of the track agentData holds.
	static void invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
	// The warp time of the track agentData holds; the entry is not read.
	static void warpTime(hkpAgentEntry* entry, hkpAgentData* agentData, float oldTime, float newTime,
		const hkpCollisionInput& input);
};
