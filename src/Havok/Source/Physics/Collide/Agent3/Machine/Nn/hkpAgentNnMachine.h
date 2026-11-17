#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"

#include <stdint.h>

class hkMotionState;
class hkpAgent3Input;
class hkpAgentNnTrack;
class hkpCollisionDispatcher;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpLinkedCollidable;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

// The bodies and transforms of an Nn machine's process for one agent entry's two collidables.
struct hkAgentNnMachineBodyTemp {
	hkpCdBody m_bodyA;
	hkpCdBody m_bodyB;
	hkTransform m_transA;
	hkTransform m_transB;
};

// Invalidates the time of impact the entry's agent cached.
void hkAgentNnMachine_InvalidateTimInAgent(hkpAgentNnEntry* entry, const hkpCollisionInput& input);
// The agent entry linking collA and collB, found in the collision entries of the one with fewer, or null when they
// have no agent.
hkpAgentNnEntry* hkAgentNnMachine_FindAgent(const hkpLinkedCollidable* collA, const hkpLinkedCollidable* collB);
// Hands out the next agent entry of the track.
hkpAgentNnEntry* hkAgentNnMachine_AllocateEntry(hkpAgentNnTrack& track);
// Copies an entry into a fresh one and relinks both collidables' collision entries.
hkpAgentNnEntry* hkAgentNnMachine_CopyAndRelinkAgentEntry(hkpAgentNnTrack& destTrack, hkpAgentNnEntry* entry);
// Returns an entry's HK_AGENT3_AGENT_SIZE-byte slab to the track, moving the track's last entry into it.
void hkAgentNnMachine_InternalDeallocateEntry(hkpAgentNnTrack& track, hkpAgentNnEntry* entry);
// Destroys the entry's agent, unlinks the entry from both collidables and frees its slab.
void hkAgentNnMachine_DestroyAgent(hkpAgentNnTrack& track, hkpAgentNnEntry* entry, hkpCollisionDispatcher* dispatcher,
	hkpConstraintOwner& constraintOwner);
// Moves the entry's agent in time: an entry that caches its time of impact takes the new time, or -1 when its old
// time does not match, and then the agent's warp-time function runs.
void hkAgentNnMachine_WarpTimeInAgent(hkpAgentNnEntry* entry, float oldTime, float newTime,
	const hkpCollisionInput& input);
// Builds a motion state and a cd body per level of transform shapes under body and returns the first body that is not a
// transform shape; cdBodyHasTransformFlag becomes TRANSFORM_FLAG when any was applied.
const hkpCdBody* hkAgentMachine_processTransformedShapes(const hkpCdBody* cdBody, hkpCdBody* newCdBodies,
	hkMotionState* newMotionStates, int32_t numSlots, uint8_t& cdBodyHasTransformFlag);
// Takes a fresh entry from the track, links it into both collidables' collision entries, picks the collision quality
// and lets agent type agentType create its agent behind the entry.
hkpAgentNnEntry* hkAgentNnMachine_CreateAgent(hkpAgentNnTrack& track, hkpLinkedCollidable* collA,
	const hkpCdBody* firstNonTransformBodyA, hkpLinkedCollidable* collB, const hkpCdBody* firstNonTransformBodyB,
	uint8_t cdBodyHasTransformFlag, int32_t agentType, const hkpProcessCollisionInput& input, hkpContactMgr* mgr);
// Walks every sector's entries to check the track.
void hkAgentNnMachine_AssertTrackValidity(hkpAgentNnTrack& track);
void hkAgentNnMachine_GetAgentType(const hkpCdBody* cdBodyA, const hkpCdBody* cdBodyB,
	const hkpProcessCollisionInput& input, int32_t& agentTypeOut, int32_t& isFlippedOut);
// Runs the agent's update-filter function on the entry's bodies, through their transform shapes.
void hkAgentNnMachine_UpdateShapeCollectionFilter(hkpAgentNnEntry* entry, const hkpCollisionInput& input,
	hkpConstraintOwner& constraintOwner);
void hkAgentNnMachine_DestroyTrack(hkpAgentNnTrack& track, hkpCollisionDispatcher* dispatcher,
	hkpConstraintOwner& constraintOwner);
// Runs the entry's agent on its two collidables per the entry's stream command.
void hkAgentNnMachine_ProcessAgent(hkpAgentNnEntry* entry, const hkpProcessCollisionInput& input,
	hkpProcessCollisionOutput& output, hkpContactMgr* mgr);
void hkAgentNnMachine_AppendTrack(hkpAgentNnTrack& track, hkpAgentNnTrack& appendee);
// Processes every agent of the track and hands the contact points each one writes to its pair's contact manager;
// hkpSimulation::collideInternal calls it per island.
void hkAgentNnMachine_ProcessTrack(hkpConstraintOwner* owner, hkpAgentNnTrack& agentTrack,
	const hkpProcessCollisionInput& input);
// out takes in's bodies at the step's start time.
void hkAgentNnMachine_initInputAtTime(hkpAgent3Input& in, hkAgentNnMachineBodyTemp& temp, hkpAgent3Input& out);
// Gives the track's last entry slot back. hkpCollide.lib declares it as (hkpAgentNnTrack&, int); this version takes no
// int because the int is never read.
void hkAgentNnMachine_DeallocateLastEntry(hkpAgentNnTrack& track);
// Takes no hkpContactMgr* parameter; the manager is read from the entry.
void hkAgentNnMachine_InlineProcessAgent(hkpAgentNnEntry* entry, const hkpProcessCollisionInput& input, int32_t& numTim,
	hkpProcessCollisionOutput& output);
// Havok SDK signature (hkpAgentNnMachine.h): whether entry lies in one of the track's sectors.
bool hkAgentNnMachine_IsEntryOnTrack(hkpAgentNnTrack& track, hkpAgentEntry* entry);

// An agent entry whose agent caches the time of impact it last computed; the agent's own data follows the entry.
struct hkpAgentNnMachineTimEntry : hkpAgentNnEntry {
	float m_timeOfSeparatingNormal;
	hkVector4 m_separatingNormal;
};

// An agent entry padded out so that the agent's own data starts on a 16-byte boundary.
struct hkpAgentNnMachinePaddedEntry : hkpAgentNnEntry {
	uint32_t m_padOutTo16BytesAlignment[2];
};
