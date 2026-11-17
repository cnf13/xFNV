#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"

class hkpBroadPhase;
class hkpCollidable;
class hkpCollisionFilter;
class hkpFixedBufferRayHitCollector;
struct hkpWorldRayCastCommand;
struct hkpWorldRayCastInput;

// The collector of the CPU world ray cast job: it receives the broad phase handles, k-d tree hits or AABB tree leaves
// the ray reaches and casts the ray against each collidable, writing the command's results.
class hkCpuWorldRayCastCollector : public hkpBroadPhaseCastCollector,
                                   public hkKdTreeCastCollector,
                                   public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector,
                                   public hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector {
public:
    // No hit yet, the early out at 1, no work filter and no fixed buffer collector.
    hkCpuWorldRayCastCollector() {
        m_hitFraction = 1.0f;
        m_workInput.m_filterInfo = 0;
        m_workInput.m_rayShapeCollectionFilter = nullptr;
        m_fixedBufferRayHitCollector = nullptr;
        m_hit.m_bool = false;
    }

    ~hkCpuWorldRayCastCollector() override;

    float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;
    // hkKdTreeCastCollector's vtable at +0x4.
    hkBool* addMidPhaseHit(hkBool* arResult, hkPrimitiveId primitiveId) override;
    // hkKdTreeCastCollector's vtable at +0x4.
    hkBool* hasHit(hkBool* arResult) override;
    // hkKdTreeCastCollector's vtable at +0x4.
    float getHitFraction() override;
    // hkAabbTreeQueryUtils<hkAabbTreeAabb, uint16_t>::Collector's vtable at +0x8.
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) override;
    // hkAabbTreeQueryUtils<hkAabbTreeAabb16, uint16_t>::Collector's vtable at +0xc.
    float addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) override;

    // Casts the ray against apCollidable when it has a shape and m_filter's ray-collidable test passes for
    // m_originalInput: the work input in the collidable's space, the hits into m_fixedBufferRayHitCollector or the next
    // free result, m_earlyOutHitFraction refreshed. The hkBool result is written through arResult, which is returned;
    // addMidPhaseHit passes its own result pointer on. addBroadPhaseHandle, addMidPhaseHit and both addLeaf call it.
    hkBool* testCollidable(hkBool* arResult, const hkpCollidable* apCollidable);

    // DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size); it also settles the four
    // bases' own.
    static void operator delete(void* apMemory);

    const hkpCollisionFilter* m_filter;
    float m_hitFraction;
    hkBool m_hit;
    uint8_t padding19[3];
    hkpWorldRayCastCommand* m_command;
    const hkpWorldRayCastInput* m_originalInput;
    hkpWorldRayCastOutput* m_result;
    hkpWorldRayCastOutput* m_nextFreeResult;
    // hkpShapeRayCastInput is 16-byte aligned (its hkVector4 members), so the bytes after m_nextFreeResult and after
    // m_fixedBufferRayHitCollector are the compiler's alignment padding.
    hkpShapeRayCastInput m_workInput;
    hkpFixedBufferRayHitCollector* m_fixedBufferRayHitCollector;
    hkVector4 m_earlyOutHitFraction;
};

// Casts the world ray cast job's commands and bundles, then finishes the job.
hkJobQueue::JobStatus hkCpuWorldRayCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
// Sets apCollector up for apCommand (its m_command, m_originalInput, results, the work input's filter info and the
// filter's ray shape collection filter when the input enables it), constructs an hkpFixedBufferRayHitCollector in
// apFixedBufferCollector (with the command's results) when the command uses a collector, then casts the command's ray
// through apBroadPhase into apCollector. hkCpuWorldRayCastJob calls it.
void castRayBroadPhase(
    const hkpBroadPhase* apBroadPhase,
    hkpWorldRayCastCommand* apCommand,
    hkCpuWorldRayCastCollector* apCollector,
    hkpFixedBufferRayHitCollector* apFixedBufferCollector
);
