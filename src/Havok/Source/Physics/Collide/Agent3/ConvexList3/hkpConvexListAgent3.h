#pragma once

#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

class hkpCdBody;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkStatisticsCollector;

// The agent data of a convex list agent: how many steps it has stayed in stream (list) mode, and the track of the
// list's sub-agents.
struct hkConvexListAgent3Data {
	int16_t m_inStreamModeCounter;
	hkpAgent1nTrack m_agentTrack;
};

// The stream agent between a convex list and a convex shape or another convex list.
class hkConvexListAgent3 {
public:
	// The agent's process: the convex list against the other body, adding its contact points to output.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& output);
	// Delegates to hkPredGskAgent3::cleanup, and the agent's function table holds it.
	static hkpAgentData* cleanup(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// Removes the contact point idToRemove: only an agent in GSK mode has contact points of its own. idToRemove is a
	// hkContactPointId.
	static void removePoint(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToRemove);
	// Registers the agent for a convex shape or a convex list against a convex list.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);

	// The rest of the agent's function table, which initAgentFunc fills.
	static hkpAgentData* create(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
	// Overwrites the first argument with the second.
	static void destroy(hkpAgentEntry* entry, hkpAgentData* agentData, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner, hkpCollisionDispatcher* dispatcher);
	static void commitPotential(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToCommit);
	static void createZombie(hkpAgentEntry* entry, hkpAgentData* agentData, uint16_t idToConvert);
	static void updateFilter(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCdBody& bodyA,
		const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// Reports the sub-agents of an agent in stream mode to collector.
	static void calcStatistics(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// Invalidates the times of impact of the sub-agents of an agent in stream mode.
	static void invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
	// The first argument is not used.
	static void warpTime(hkpAgentEntry* entry, hkpAgentData* agentData, float oldTime, float newTime,
		const hkpCollisionInput& input);
	static void sepNormal(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4& separatingNormalOut);
	// Fills f with the agent's functions.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);
	// The track of the sub-agents of an agent in stream mode.
	static hkpAgent1nTrack* getAgent1nTrack(const hkpAgentEntry* entry, hkpAgentData* data);
	static const hkpAgent1nTrack* getAgent1nTrack(const hkpAgentEntry* entry, const hkpAgentData* data);
};
