#pragma once

#include "Reconstruction.hpp"

class hkpCollisionInput;
class hkpEntity;

// A sort key and the index it belongs to; entityBatchRecalcAabb sorts the entities by it (hkpdynamics.pdb sizeof 8;
// the game PDB has hkAlgorithm::quickSort<hkValueIndexPair> and hkLocalBuffer<hkValueIndexPair>).
class hkValueIndexPair {
public:
	// Orders the pairs by value (the SDK's inline; the exe inlines it in its quickSortRecursive copy, 0xD1B1D0).
	bool operator<(const hkValueIndexPair& p1) const { return m_value < p1.m_value; }

	uint32_t m_value;
	uint32_t m_index;
};

namespace hkAlgorithm {
// The SDK's quickSortRecursive<hkValueIndexPair, hkAlgorithm::less<hkValueIndexPair>> as the exe compiled it, with
// the pair's operator< inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by value, an unsigned compare
// (cdecl, ret 0, 0xD1B1D0). The comparator (an empty class) travels in a 4-byte stack slot of which the caller writes
// only the low byte, passed on to the recursive call, so it is that byte here: an empty class argument carries no
// bytes in C++. eax is left holding the last right index, or the last recursive call's eax.
int32_t quickSortRecursive(hkValueIndexPair* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
} // namespace hkAlgorithm

// Havok SDK: batch operations on entities' broad-phase bounding boxes. Only the entry points src calls are declared.
class hkpEntityAabbUtil {
public:
	// Marks each entity's cached aabb invalid.
	static void entityBatchInvalidateAabb(hkpEntity* const* entityBatch, int32_t numEntities);
	// Recomputes each entity's broad-phase aabb with half the collision input's tolerance, and for a shape with
	// child shape aabbs the sorted child aabbs and keys.
	static void entityBatchRecalcAabb(const hkpCollisionInput* collisionInput, hkpEntity* const* entityBatch,
		int32_t numEntities);
};
