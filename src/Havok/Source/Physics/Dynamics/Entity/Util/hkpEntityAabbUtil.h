#pragma once

#include <cstdint>

class hkpCollisionInput;
class hkpEntity;

// A sort key and the index it belongs to; entityBatchRecalcAabb sorts the entities by it.
class hkValueIndexPair {
public:
	// Orders the pairs by value.
	bool operator<(const hkValueIndexPair& p1) const { return m_value < p1.m_value; }

	uint32_t m_value;
	uint32_t m_index;
};

namespace hkAlgorithm {
// quickSortRecursive<hkValueIndexPair, hkAlgorithm::less<hkValueIndexPair>>: sorts apArr[aiLow..aiHigh] in place
// (inclusive bounds) by value, an unsigned compare. aiLess stands for the comparator (an empty class) and is passed on
// to the recursive call. Returns the last right index, or the last recursive call's result.
int32_t quickSortRecursive(hkValueIndexPair* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);
} // namespace hkAlgorithm

// Batch operations on entities' broad-phase bounding boxes. Only the entry points this code calls are declared.
class hkpEntityAabbUtil {
public:
	// Marks each entity's cached aabb invalid.
	static void entityBatchInvalidateAabb(hkpEntity* const* entityBatch, int32_t numEntities);
	// Recomputes each entity's broad-phase aabb with half the collision input's tolerance, and for a shape with
	// child shape aabbs the sorted child aabbs and keys.
	static void entityBatchRecalcAabb(const hkpCollisionInput* collisionInput, hkpEntity* const* entityBatch,
		int32_t numEntities);
};
