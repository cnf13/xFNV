#pragma once

#include <cstdint>

class hkpCollisionInput;
class hkpEntity;

// A sort key and the index it belongs to; entityBatchRecalcAabb sorts the entities by it.
class hkValueIndexPair {
public:
    // Orders the pairs by value.
    bool operator<(const hkValueIndexPair& p1) const {
        return m_value < p1.m_value;
    }

    uint32_t m_value;
    uint32_t m_index;
};

// Batch operations on entities' broad-phase bounding boxes. Only the entry points this code calls are declared.
class hkpEntityAabbUtil {
public:
    // Marks each entity's cached aabb invalid.
    static void entityBatchInvalidateAabb(hkpEntity* const* entityBatch, int32_t numEntities);
    // Recomputes each entity's broad-phase aabb with half the collision input's tolerance, and for a shape with
    // child shape aabbs the sorted child aabbs and keys.
    static void
    entityBatchRecalcAabb(const hkpCollisionInput* collisionInput, hkpEntity* const* entityBatch, int32_t numEntities);
};
