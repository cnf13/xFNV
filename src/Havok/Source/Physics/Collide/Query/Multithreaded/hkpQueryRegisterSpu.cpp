#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbQueryUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkCpuWorldGetClosestPointsCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuMoppAabbJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuPairGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuPairLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuWorldLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobQueueUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuAutoRayBundleJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuKdTreeBuildJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuShapeRaycastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuWorldRaycastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobQueueUtils.h"

static hkJobQueue::JobStatus hkCpuAabbTreeAabbJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
static hkJobQueue::JobStatus hkCpuKdTreeAabbJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
// The SDK's hkAabbTreeQueryUtils<hkAabbTreeAabb16, hkUint16>::queryAabb
// with a write iterator (hkAabbTreeQueryUtils.h): queryAabbInternal (queryAabbInternal) with no collector and no array.
// hkCpuAabbTreeAabbJob passes the job's m_aabbTreeInt16 and adds the result to its hit count.
static int32_t queryAabbInt16ToBuffer(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    void*** hitArray,
    int32_t hitCapacity
);
// The SDK's hkAabbTreeQueryUtils<hkAabbTreeAabb, hkUint16>::queryAabb
// with a write iterator (hkAabbTreeQueryUtils.h): queryAabbInternal (queryAabbInternal) with no collector and no array.
// hkCpuAabbTreeAabbJob passes the job's m_aabbTreeFp.
static int32_t queryAabbFpToBuffer(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabb& aabb,
    void*** hitArray,
    int32_t hitCapacity
);
// Equivalent of hkAabbTreeQueryUtils<hkAabbTreeAabb, hkUint16>::queryAabbInternal (hkAabbTreeQueryUtils.h): reads the
// tree's root node, the box's two vectors, and one of the three outputs, as queryAabbInternal does.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabb& aabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector* collector,
    hkArrayBase<void>* hitArray,
    void*** hitIterator,
    int32_t hitCapacity
);

// Tables of process job functions, indexed by the index the job carries; they start zeroed and the register functions
// fill them. The ray cast table holds the 8 entries its register function sets, the collision query one its 7.
using KdTreeProcessFuncs = hkJobQueue::ProcessJobFunc[8];
static KdTreeProcessFuncs s_KdTreeProcessFuncs{};
using CollisionQueryProcessFuncs = hkJobQueue::ProcessJobFunc[7];
static CollisionQueryProcessFuncs s_collisionQueryProcessFuncs{};

// The SDK's hkCpuAabbTreeAabbJob: each command's box is queried against whichever of the job's float AABB tree,
// 16-bit AABB tree and kd-tree are set, all three writing through one cursor into the command's results.
static hkJobQueue::JobStatus hkCpuAabbTreeAabbJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuAabbTreeAabb", nullptr);
    // The queue entry holds the job: both are hkJobs.
    const hkJob& baseJob = nextJobOut;
    const hkpAabbTreeAabbJob& job = static_cast<const hkpAabbTreeAabbJob&>(baseJob);
    hkpAabbTreeAabbCommand* command = job.m_aabbCommandArray;
    for (int32_t i = job.m_numAabbCommands; i > 0; i--, command++) {
        void** results = command->m_results;
        int32_t maxHits = command->m_resultsCapacity;
        command->m_numResultsOut = 0;
        if (job.m_aabbTreeFp) {
            const int32_t numHits = queryAabbFpToBuffer(job.m_aabbTreeFp, command->m_aabb, &results, maxHits);
            command->m_numResultsOut += numHits;
            maxHits -= numHits;
        }
        if (job.m_aabbTreeInt16) {
            const int32_t numHits = queryAabbInt16ToBuffer(job.m_aabbTreeInt16, command->m_aabb, &results, maxHits);
            command->m_numResultsOut += numHits;
            maxHits -= numHits;
        }
        if (job.m_kdTree) {
            // The results buffer is untyped: the same cursor is passed to the kd-tree query, which writes hkUlong hits
            // through it.
            command->m_numResultsOut += hkKdTreeAabbQueryUtils::queryAabbIterative(
                job.m_kdTree,
                command->m_aabb,
                reinterpret_cast<hkUlong**>(&results),
                maxHits
            );
        }
    }
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// The SDK's hkCpuKdTreeAabbJob: each command's box is queried against each of the job's kd-trees that is set, all
// writing through one cursor into the command's results.
static hkJobQueue::JobStatus hkCpuKdTreeAabbJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeAabb", nullptr);
    // The queue entry holds the job: both are hkJobs.
    const hkJob& baseJob = nextJobOut;
    const hkpKdTreeAabbJob& job = static_cast<const hkpKdTreeAabbJob&>(baseJob);
    hkpKdTreeAabbCommand* command = job.m_aabbCommandArray;
    for (int32_t i = job.m_numAabbCommands; i > 0; i--, command++) {
        hkPrimitiveId* results = command->m_results;
        int32_t maxHits = command->m_resultsCapacity;
        command->m_numResultsOut = 0;
        for (int32_t t = 0; t < job.m_numTrees; t++) {
            if (job.m_trees[t]) {
                const uint32_t numHits = hkKdTreeAabbQueryUtils::queryAabbIterative(
                    job.m_trees[t],
                    command->m_aabb,
                    &results,
                    maxHits
                );
                command->m_numResultsOut += numHits;
                maxHits -= numHits;
            }
        }
    }
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

static int32_t queryAabbInt16ToBuffer(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabb& aabb,
    void*** hitArray,
    int32_t hitCapacity
) {
    // Returns the null tree pointer itself: 0.
    if (tree) {
        return queryAabbInternal(tree, aabb, nullptr, nullptr, hitArray, hitCapacity);
    }
    return 0;
}

static int32_t queryAabbFpToBuffer(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabb& aabb,
    void*** hitArray,
    int32_t hitCapacity
) {
    // Returns the null tree pointer itself, 0.
    if (tree) {
        return queryAabbInternal(tree, aabb, nullptr, nullptr, hitArray, hitCapacity);
    }
    return 0;
}

// The 16-bit-index, float-volume twin of queryAabbInternal: the same walk and outputs over the nodes' float boxes.
static int32_t queryAabbInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabb& aabb,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector* collector,
    hkArrayBase<void>* hitArray,
    void*** hitIterator,
    int32_t hitCapacity
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
    const Tree::Node* node = tree->m_rootNode;
    if (!node) {
        return 0;
    }
    int32_t numHits = 0;
    if (!collector && !hitIterator && !hitArray) {
        return numHits;
    }
    for (;;) {
        const __m128 nodeMin = node->m_volume.m_min.m_quad;
        const __m128 nodeMax = node->m_volume.m_max.m_quad;
        const uint16_t skipIdx = node->m_skipNodeIdx;
        const Tree::Node* next = (skipIdx == Tree::s_invalidIdx) ? nullptr : tree->m_nodeManager.getNode(skipIdx);
        if ((_mm_movemask_ps(
                 _mm_and_ps(_mm_cmple_ps(aabb.m_min.m_quad, nodeMax), _mm_cmple_ps(nodeMin, aabb.m_max.m_quad))
             ) &
             7) == 7) {
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            if (collector) {
                if (collector->addLeaf(node) == 0.0f) {
                    return numHits;
                }
            }
            else if (hitIterator) {
                *(*hitIterator)++ = node->m_leafData.m_userData;
                if (numHits >= hitCapacity) {
                    return numHits;
                }
            }
            else {
                // hitArray is the SDK's hkArray<void*>, typed hkArrayBase<void> as in queryAabbInternal.
                void* userData = node->m_leafData.m_userData;
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (hitArray->m_size == hitArray->getCapacity()) {
                    hkArrayUtil::_reserveMore(router.heap(), hitArray, sizeof(void*));
                }
                static_cast<void**>(hitArray->m_data)[hitArray->m_size] = userData;
                ++hitArray->m_size;
            }
            ++numHits;
        }
        node = next;
        if (!node) {
            return numHits;
        }
    }
}

// Registers the seven collision query job functions and the queue's pop and finish callbacks with queue for job
// type 2.
void hkpCollisionQueryJobQueueUtils::registerWithJobQueue(hkJobQueue* queue) {
    hkJobQueue::hkJobHandlerFuncs funcs;
    funcs.m_processJobFuncs = s_collisionQueryProcessFuncs;
    funcs.m_numProcessJobFuncs = 7;
    funcs.m_popJobFunc = &hkpCollisionQueryJobQueueUtils::popCollisionJob;
    s_collisionQueryProcessFuncs[3] = &hkCpuPairLinearCastJob;
    s_collisionQueryProcessFuncs[4] = &hkCpuWorldLinearCastJob;
    s_collisionQueryProcessFuncs[0] = &hkCpuMoppAabbQueryJob;
    s_collisionQueryProcessFuncs[1] = &hkCpuPairGetClosestPointsJob;
    s_collisionQueryProcessFuncs[2] = &hkCpuWorldGetClosestPointsJob;
    s_collisionQueryProcessFuncs[5] = &hkCpuKdTreeAabbJob;
    s_collisionQueryProcessFuncs[6] = &hkCpuAabbTreeAabbJob;
    funcs.m_finishJobFunc = &hkpCollisionQueryJobQueueUtils::finishCollisionJob;
    queue->registerJobHandler(HK_JOB_TYPE_COLLISION_QUERY, funcs);
}

// Registers the eight ray cast query job functions and the queue's pop and finish callbacks with queue for job
// type 3.
void hkpRayCastQueryJobQueueUtils::registerWithJobQueue(hkJobQueue* queue) {
    hkJobQueue::hkJobHandlerFuncs funcs;
    funcs.m_processJobFuncs = s_KdTreeProcessFuncs;
    funcs.m_numProcessJobFuncs = 8;
    funcs.m_popJobFunc = &hkpRayCastQueryJobQueueUtils::popRayCastQueryJob;
    s_KdTreeProcessFuncs[0] = &hkCpuShapeRayCastJob;
    s_KdTreeProcessFuncs[1] = &hkCpuWorldRayCastJob;
    s_KdTreeProcessFuncs[2] = &hkCpuKdTreeBuildJobFast;
    s_KdTreeProcessFuncs[3] = &hkCpuKdTreeBuildFastDistributedJob;
    s_KdTreeProcessFuncs[4] = &hkCpuKdTreeBuildFastDistributedSubJob;
    s_KdTreeProcessFuncs[5] = &hkCpuKdTreeBuildSetupJob;
    s_KdTreeProcessFuncs[6] = &hkCpuKdTreeBuildCoordinatorJob;
    s_KdTreeProcessFuncs[7] = &hkCpuAutoRayBundleJob;
    funcs.m_finishJobFunc = &hkpRayCastQueryJobQueueUtils::finishRayCastQueryJob;
    queue->registerJobHandler(HK_JOB_TYPE_RAYCAST_QUERY, funcs);
}
