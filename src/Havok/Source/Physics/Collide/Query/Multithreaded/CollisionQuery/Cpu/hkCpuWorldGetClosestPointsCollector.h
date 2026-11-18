#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkpCollidable;
class hkpFixedBufferCdPointCollector;
class hkpGroupFilter;

// Job function; its body is in hkpCpuWorldGetClosestPointsJob.cpp.
hkJobQueue::JobStatus hkCpuWorldGetClosestPointsJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);

// Collects broadphase, AABB-tree and KD-tree candidates for closest-point queries.
class hkCpuWorldGetClosestPointsCollector
  : public hkpBroadPhaseCastCollector,
    public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector,
    public hkKdTreeCastCollector {
public:
    // hkpLinearCastCollisionInput's constructor: the penetration starts at HK_REAL_EPSILON.
    hkCpuWorldGetClosestPointsCollector() {
        m_input.m_maxExtraPenetration = 1.1920929e-07f;
    }
    ~hkCpuWorldGetClosestPointsCollector() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadphaseHandle, int32_t castIndex) override;
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) override;
    // hkKdTreeCastCollector's vtable at +0x8.
    hkBool addMidPhaseHit(hkPrimitiveId primitiveId) override;
    // hkKdTreeCastCollector's vtable at +0x8.
    hkBool hasHit() override;
    // hkKdTreeCastCollector's vtable at +0x8.
    float getHitFraction() override;

    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    float testCollidable(const hkpCollidable* collidable);

    // DeletingDestruct's free (ourFree); it also settles the three bases' own.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    const hkpGroupFilter* m_filter;
    hkpFixedBufferCdPointCollector* m_castCollector;
    const hkpCollidable* m_collidable;
    hkpShapeType m_shapeType;
    // 4 bytes of padding: m_input is 16-byte aligned (its hkVector4 members), so it starts at 0x20.
    hkpLinearCastCollisionInput m_input;
};
