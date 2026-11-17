#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkStatisticsCollector;
class hkpAgent3ProcessInput;
struct hkpAgent3RootMotion;
struct hkpAgentEntry;
class hkpCdBody;
class hkpCollidable;
class hkpCollisionInput;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpShapeContainer;

// The SDK's hkpAgentData is void: the bytes of an agent's data, in front of its entry.
typedef void hkpAgentData;

// The stream agent between two shape collections or bounding volume trees: its registration, its process and the
// AABB helpers the process uses.
class hkpCollectionCollectionAgent3 {
public:
	// Orders two quantised child AABBs by their lowest corner, the axis the sweep walks. The exe's instantiation
	// takes the empty comparator by value in a 4-byte stack slot of which the callers write only the low byte (0), so
	// it is that byte here: an empty class argument carries no bytes in C++ (as hkSort.h's quickSort aiLess).
	struct AabbUint32Less {
		bool operator()(const hkAabbUint32& arA, const hkAabbUint32& arB) const { return arA.m_min[0] < arB.m_min[0]; }

		uint8_t m_byte;
	};

	// The key of a quantised hit list entry: the four bytes after the lowest corner, where an hkAabbUint32 holds its
	// expansion bytes.
	static void setHitListKey(hkAabbUint32& arAabb, uint32_t auiKey);
	// Packs a float box into the input's quantised AABB space.
	static void quantiseAabb(const hkpCollisionInput& arInput, const hkAabb& arAabb, hkAabbUint32& arOut);
	// Grows a child shape's box by the root body's motion over the step (the exe inlines it at every call site).
	static void growAabbByRootMotion(const hkpAgent3RootMotion& arMotion, hkAabb& arAabb);
	// One quantised AABB per shape key of the container. Not named in the PDB or IDA; name chosen here.
	// Inferred type: aiNumKeys: int32_t: signed uses from 0xd34e62
	static void calcAabbsOfKeys(const hkpAgent3ProcessInput& arInput, const hkpShapeContainer* apShapeContainer,
		const hkpCdBody& arBodyA, const uint32_t* apuiKeys, int32_t aiNumKeys, hkAabbUint32* apAabbsOut);
	// The quantised child AABBs the collidable caches in its bounding volume data, or one box per child shape of the
	// container its shape exposes. IDA name; not in the PDB.
	static int32_t calcAabbsOfCollidable(const hkpCollidable* apCollidable, const hkpCdBody* apOverrideBody,
		const hkpShapeContainer* apShapeContainer, const hkpProcessCollisionInput* apInput, hkAabbUint32* apAabbsOut);

	// The agent's process: queries one body's children against the other's tree and narrows
	// the pairs.
	static hkpAgentData* process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry, hkpAgentData* agentData,
		hkVector4* separatingNormal, hkpProcessCollisionOutput& output);
	// Fills f with the agent's functions and makes it predictive.
	static void initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f);
	// The agent3 table's filter update: re-filters the child pairs of the track agentData holds.
	static void updateFilter(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCdBody& bodyA,
		const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr,
		hkpConstraintOwner& constraintOwner);
	// The agent3 table's statistics of the track agentData holds.
	static void calcStatistics(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// The agent3 table's time of impact reset of the track agentData holds.
	static void invalidateTim(hkpAgentEntry* entry, hkpAgentData* agentData, const hkpCollisionInput& input);
	// The agent3 table's warp time: the entry is not used, the agent data is the track.
	static void warpTime(hkpAgentEntry* entry, hkpAgentData* agentData, float oldTime, float newTime,
		const hkpCollisionInput& input);
	// Registers the agent for the pairs of collections and bounding volume trees.
	static void registerAgent3(hkpCollisionDispatcher* dispatcher);

	static inline char g_agentRegistered;
};

// A copy of the four 1.0 lanes the exe reads by address (the PDB's hkQuadReal1111): the linker folded this file's
// copy with hkpTriangleShape's, hkp1dBilateralConstraintBuildJacobian's and hkpEntityAabbUtil's into one.
inline hkQuadReal hkQuadReal1111_0{1.0f, 1.0f, 1.0f, 1.0f};
