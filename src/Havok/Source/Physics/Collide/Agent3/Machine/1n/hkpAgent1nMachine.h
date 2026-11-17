#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Reconstruction.hpp"

class hkStatisticsCollector;
class hkpAgent3Input;
class hkpAgent3ProcessInput;
class hkpCollisionInput;
class hkpConstraintOwner;
class hkpContactMgr;
class hkpShapeContainer;

// The pair of shape keys a 1n agent entry is for. The keys are the SDK's hkpShapeKey, an unsigned 32-bit integer.
class hkShapeKeyPair {
public:
	// The SDK's order: by key A, then by key B, unsigned.
	bool operator<(const hkShapeKeyPair& p1) const {
		return (m_shapeKeyA < p1.m_shapeKeyA) || (m_shapeKeyA == p1.m_shapeKeyA && m_shapeKeyB < p1.m_shapeKeyB);
	}

	uint32_t m_shapeKeyA;
	uint32_t m_shapeKeyB;
};

// The header of one agent in a 1n track: the stream entry and the shape keys it collides.
struct hkpAgent1nMachineEntry : hkpAgentEntry {
	hkShapeKeyPair m_shapeKeyPair;
};

// An entry padded to 16 bytes, so that the agent's data that follows it is aligned.
struct hkpAgent1nMachinePaddedEntry : hkpAgent1nMachineEntry {
	uint32_t m_padUpTo16Bytes;
};

// An entry whose agent caches its time of impact and separating normal; the agent's data follows it.
struct hkpAgent1nMachineTimEntry : hkpAgent1nMachineEntry {
	float m_timeOfSeparatingNormal;
	hkVector4 m_separatingNormal;
};

// What a visitor of a 1n track's agents gets besides the entry: the two bodies and shape containers, the input and
// the contact manager and constraint owner, and the visitor's own data (hkpcollide.pdb, sizeof 32; the visitors
// 0xDA3F90 and 0xDA40E0 read +0x0..+0x18).
struct hkpAgent1nMachine_VisitorInput {
	const hkpCdBody* m_bodyA;
	const hkpCdBody* m_collectionBodyB;
	const hkpShapeContainer* m_containerShapeA;
	const hkpShapeContainer* m_containerShapeB;
	const hkpCollisionInput* m_input;
	hkpContactMgr* m_contactMgr;
	hkpConstraintOwner* m_constraintOwner;
	void* m_clientData;
};

// Finds the entry of the track for the key pair and its agent's data (cdecl, ret 0, 0xDA3740). The exe has no caller
// of it; the PDB names it.
hkpAgent1nMachineEntry* hkAgent1nMachine_FindAgent(hkpAgent1nTrack& agentTrack, hkShapeKeyPair keyPair,
	hkpAgentData** agentDataOut);
// Havok SDK signature: welds the potential contacts the track's agents added to output (cdecl, ret 0, 0xDA3930; eax
// holds no result, the caller 0xDA4AA0 ignores it).
void hkAgent1nMachine_Weld(hkpAgent3Input& input, const hkpShapeContainer* shapeCollection,
	hkpProcessCollisionOutput& output);
// The SDK's file-local visitor that updates the filter of a 1n track entry's agent (shape key B at +0x8): returns the
// next entry, or the entry itself after destroying its agent when the filter now rejects the pair.
// Its callers pass it to hkAgent1nMachine_VisitAllAgents.
void* hkAgent1nMachine_UpdateShapeCollectionFilterVisitor(hkpAgent1nMachine_VisitorInput& vin,
	hkpAgent1nMachineEntry* entry, void* agentData);
// The same for an Nm track's entry, whose key pair names a child of both containers.
void* hkAgentNmMachine_UpdateShapeCollectionFilterVisitor(hkpAgent1nMachine_VisitorInput& vin,
	hkpAgent1nMachineEntry* entry, void* agentData);
// Cleans up the child agents of the track and frees its sectors. The agent data of a 1n
// agent is the track itself.
void hkAgent1nMachine_Destroy(hkpAgent1nTrack& agentTrack, hkpCollisionDispatcher* dispatcher, hkpContactMgr* mgr,
	hkpConstraintOwner& constraintOwner);
// Starts the track with one sector holding only the end-of-stream entry.
void hkAgent1nMachine_Create(hkpAgent1nTrack& agentTrack);
// Invalidates the cached time of impact of every agent of the track.
void hkAgent1nMachine_InvalidateTim(hkpAgent1nTrack& agentTrack, const hkpCollisionInput& input);
// Warps the cached times of the track's child agents.
void hkAgent1nMachine_WarpTime(hkpAgent1nTrack& agentTrack, float oldTime, float newTime,
	const hkpCollisionInput& input);
void hkAgent1nMachine_UpdateShapeCollectionFilter(hkpAgent1nTrack& agentTrack, hkpAgent1nMachine_VisitorInput& vin);
void hkAgentNmMachine_UpdateShapeCollectionFilter(hkpAgent1nTrack& agentTrack, hkpAgent1nMachine_VisitorInput& vin);
// Callers: hkBvTreeAgent3::process 0xD2D890, the list, convex list and bv-tree stream agents.
void hkAgent1nMachine_Process(hkpAgent1nTrack& agentTrack, const hkpAgent3ProcessInput& inputIn,
	const hkpShapeContainer* shapeContainerB, const uint32_t* newKeys, hkpProcessCollisionOutput& output);
void hkAgentNmMachine_Process(hkpAgent1nTrack& agentTrack, const hkpAgent3ProcessInput& inputIn,
	const hkpShapeContainer* shapeContainerA, const hkpShapeContainer* shapeContainerB, const hkShapeKeyPair* newKeys,
	hkpProcessCollisionOutput& output);
// Not named in the PDB or IDA; name chosen here. Register arguments: oldStart in edx, info in edi, newStart in esi;
// size on the stack.
void updatePotentialContactPointers(const uint8_t* oldStart, int32_t size,
	hkpProcessCollisionOutput::PotentialInfo* info, uint8_t* newStart);

// The result of comparing two shape key pairs of a track. Values from Havok's release hkpcollide.pdb
// (hkpAgent1nMachine.cpp).
enum hkContactPairCompare : int32_t {
	HK_CONTACT_PAIR_OK = 0x0,
	HK_CONTACT_PAIR_REJECT = 0x1,
	HK_CONTACT_PAIR_CONFLICT = 0x2,
};

// The bodies and transforms of both sides' child shapes in a 1n machine's Nm process. Members and layout from
// Havok's release hkpcollide.pdb (sizeof 0xA0; the game PDB places it in hkpAgent1nMachineProcess.cpp).
struct hkAgentNmMachineBodyTemp {
	hkpCdBody m_bodyA;
	hkpCdBody m_bodyB;
	hkTransform m_transA;
	hkTransform m_transB;
};

// A 1n process's local shape key pair: only B's key (the PDB's operator==, operator> and writeTo use it). Members
// and layout from Havok's release hkpcollide.pdb (sizeof 4; the game PDB places it in hkpAgent1nMachineProcess.cpp).
class hkShapeKeyPairLocal1n {
public:
	hkPadSpu<uint32_t> m_shapeKeyB;
};

// An Nm process's local shape key pair: A's and B's keys (the PDB's operator==, operator> and writeTo use them).
// Members and layout from Havok's release hkpcollide.pdb (sizeof 8; the game PDB places it in
// hkpAgent1nMachineProcess.cpp).
class hkShapeKeyPairLocalNm {
public:
	hkPadSpu<uint32_t> m_shapeKeyA;
	hkPadSpu<uint32_t> m_shapeKeyB;
};
