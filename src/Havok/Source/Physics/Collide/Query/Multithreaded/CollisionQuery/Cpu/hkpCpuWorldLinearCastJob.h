#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

class hkpCollidable;
class hkpFixedBufferCdPointCollector;
class hkpGroupFilter;

hkJobQueue::JobStatus hkCpuWorldLinearCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);

// The collector of the CPU world linear cast job: it receives the broad phase handles, k-d tree hits or AABB tree
// leaves the cast reaches and casts the collidable against each of them into the fixed buffer point collector.
class hkCpuWorldLinearCastCollector
  : public hkpBroadPhaseCastCollector,
    public hkKdTreeCastCollector,
    public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector,
    public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector {
public:
    hkCpuWorldLinearCastCollector() {}
    ~hkCpuWorldLinearCastCollector() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;
    // hkKdTreeCastCollector's vtable at +0x4. Casts against the collidable primitiveId and reports a hit.
    hkBool addMidPhaseHit(hkPrimitiveId primitiveId) override;
    // hkKdTreeCastCollector's vtable at +0x4.
    hkBool hasHit() override;
    // hkKdTreeCastCollector's vtable at +0x4.
    float getHitFraction() override;
    // hkAabbTreeQueryUtils<hkAabbTreeAabb, uint16_t>::Collector's vtable at +0x8.
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) override;
    // hkAabbTreeQueryUtils<hkAabbTreeAabb16, uint16_t>::Collector's vtable at +0xc.
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) override;

    // Casts m_collidable against collidable unless it is the cast's own or has no shape; returns the early-out
    // fraction.
    float testCollidable(const hkpCollidable* collidable);

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size); it also settles the four
    // bases' own.
    static void operator delete(void* apMemory);

    const hkpGroupFilter* m_filter;
    hkpFixedBufferCdPointCollector* m_castCollector;
    const hkpCollidable* m_collidable;
    hkpShapeType m_shapeType;
    hkpLinearCastCollisionInput m_input;
};
