#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuKdTreeBuildJob.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuilder.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildingUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Util/KdTree/Mediator/hkpCollidableMediator.h"

hkJobQueue::JobStatus hkCpuKdTreeBuildJobFast(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeFastBuild", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpKdTreeBuildJobFast& job = static_cast<const hkpKdTreeBuildJobFast&>(static_cast<const hkJob&>(nextJobOut));
    hkKdTreeBuildInput::WorkingSet set;
    set.m_entriesIn = job.m_entriesIn;
    set.m_entriesOut = job.m_entriesOut;
    int32_t depth = 0;
    hkKdTreeBuildingUtils::buildTree(
        set, job.m_treeOut, job.m_projEntOut, job.m_nodeArraySize, job.m_numCollidables, job.m_numAvailableEmptyNodes,
        depth
    );
    *job.m_treeDepthOut = depth;
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Builds the k-d tree of the job's primitives: under 8 primitives in one go, storing the depth for each of the four
// branches; else splits the top of the tree into four branches and queues a sub job per branch, each with a quarter
// of the reserved empty nodes (the last one the rest).
// KEEP: each sub job goes to addJobQueueLocked (0xD04B00), whose enqueue (0xD057F0, 0xD057A0) copies a whole
// 0x80-byte queue entry (rep movsd, 0x20 dwords) into the queue: of the exe's 0x60-byte sub job nothing writes the
// bytes at +0x3, +0x8 to +0xF, +0x1C (m_collisionInput), +0x3C and +0x5C, and the copy runs 0x20 bytes past it. The
// bounds handed to splitTreeBuilding leave m_primIdHigh and m_primIdLow unwritten as well.
hkJobQueue::JobStatus hkCpuKdTreeBuildFastDistributedJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeBuildFastDistributed", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpKdTreeBuildDistributedJob& job =
        static_cast<const hkpKdTreeBuildDistributedJob&>(static_cast<const hkJob&>(nextJobOut));
    hkKdTreeBuildInput::WorkingSet set;
    set.m_entriesIn = job.m_primitivesIn;
    set.m_entriesOut = job.m_primitivesOut;
    if (job.m_numPrimitives < 8) {
        int32_t depthOut;
        hkKdTreeBuildingUtils::buildTree(
            set, job.m_treeOut, job.m_projEntOut, job.m_nodeArraySize, job.m_numPrimitives,
            job.m_numAvailableEmptyNodes, depthOut
        );
        if (job.m_branchDepthsOut) {
            job.m_branchDepthsOut[0] = depthOut;
            job.m_branchDepthsOut[1] = depthOut;
            job.m_branchDepthsOut[2] = depthOut;
            job.m_branchDepthsOut[3] = depthOut;
        }
    }
    else {
        hkKdTreeProjectedEntry bounds;
        bounds.m_min[1] = 0;
        bounds.m_min[2] = 0;
        bounds.m_min[0] = 0;
        bounds.m_max[1] = 0xFFFF;
        bounds.m_max[2] = 0xFFFF;
        bounds.m_max[0] = 0xFFFF;
        hkKdTreeBuildInput::WorkingSet buffers[4];
        hkKdTreeBuildInput::BuildRecursiveInput inputs[4];
        for (int32_t i = 0; i < 4; i++) {
            inputs[i].m_buffers = &buffers[i];
            inputs[i].m_maxNodeIdx = -1;
            inputs[i].m_allowedEmptySplits.m_mask = hkVector4Comparison_s_maskFromBits[hkVector4Comparison::MASK_XYZ];
        }
        hkKdTreeProjectedEntryBuilder::splitTreeBuilding(
            set, job.m_treeOut, job.m_nodeArraySize, job.m_numPrimitives, inputs, job.m_subJobSizes, bounds
        );

        // The branches' nodes follow the tree's top 8; each branch's size is rounded up to even with its share of
        // the empty nodes.
        hkpKdTreeBuildDistributedSubJob subJobs[4];
        const int32_t numEmptyNodesPerJob = job.m_numAvailableEmptyNodes / 4;
        int32_t nodeOffset = 8;
        for (int32_t i = 0; i < 4; i++) {
            hkpKdTreeBuildDistributedSubJob& subJob = subJobs[i];
            const hkKdTreeBuildInput::BuildRecursiveInput& input = inputs[i];
            // EXE-INLINED none: hkpKdTreeBuildDistributedSubJob::hkpKdTreeBuildDistributedSubJob
            subJob.m_jobSubType =
                static_cast<uint8_t>(hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_FAST_DISTRIBUTED_SUBJOB);
            subJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_RAYCAST_QUERY);
            subJob.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
            subJob.m_size = static_cast<uint16_t>(sizeof(hkpKdTreeBuildDistributedSubJob));
            subJob.m_threadAffinity = -1;
            subJob.m_jobDoneFlag = nullptr;
            subJob.m_semaphore = job.m_semaphore;
            subJob.m_sharedJobHeaderOnPpu = job.m_sharedJobHeaderOnPpu;
            subJob.m_treeOut = job.m_treeOut + nodeOffset;
            subJob.m_projEntOut = job.m_projEntOut;
            subJob.m_currentNodeIdx = input.m_currentNodeIdx;
            subJob.m_primitivesIn = input.m_buffers->m_entriesIn;
            subJob.m_primitivesOut = input.m_buffers->m_entriesOut;
            subJob.m_startPrimIdx = input.m_startPrimIdx;
            subJob.m_endPrimIdx = input.m_endPrimIdx;
            subJob.m_bounds = input.m_bounds;
            subJob.m_branchDepthOut = job.m_branchDepthsOut ? job.m_branchDepthsOut + i : nullptr;
            subJob.m_nodeArraySize = input.m_maxNodeIdx;
            subJob.m_numAvailableEmptyNodes =
                i < 3 ? numEmptyNodesPerJob : job.m_numAvailableEmptyNodes - 3 * numEmptyNodesPerJob;
            job.m_subJobSizes[i] += numEmptyNodesPerJob + (numEmptyNodesPerJob & 1);
            nodeOffset += job.m_subJobSizes[i];
        }
        job.m_sharedJobHeaderOnPpu->m_openJobs = 4;

        // The SDK's storage for the queue's locked data.
        alignas(16) char dynamicDataStorage[sizeof(hkJobQueue::DynamicData)];
        hkJobQueue::DynamicData* data = jobQueue.lockQueue(dynamicDataStorage);
        for (int32_t i = 0; i < 4; i++) {
            // The queue takes the job as an entry: its hkJob base is the entry's.
            jobQueue.addJobQueueLocked(
                data, static_cast<const hkJobQueue::JobQueueEntry&>(static_cast<const hkJob&>(subJobs[i])),
                hkJobQueue::JOB_HIGH_PRIORITY
            );
        }
        jobQueue.unlockQueue(data);
    }
    const hkJobQueue::JobStatus status =
        jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    HK_TIMER_END();
    return status;
}

// Builds one branch of a distributed k-d tree from the sub job's node, primitive range and bounds, and stores the
// branch's depth (plus the two levels above it) where the sub job says. The writer's destructor is an empty function,
// so it is not called after finishJobAndGetNextJob.
hkJobQueue::JobStatus
hkCpuKdTreeBuildFastDistributedSubJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeBuildFastDistributedSub", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpKdTreeBuildDistributedSubJob& job =
        static_cast<const hkpKdTreeBuildDistributedSubJob&>(static_cast<const hkJob&>(nextJobOut));
    hkKdTreeBuildInput::TreeStream treeOut;
    hkKdTreeBuildInput::WorkingSet set;
    hkKdTreeBuildInput::BuildRecursiveInput input;
    set.m_entriesIn = job.m_primitivesIn;
    set.m_entriesOut = job.m_primitivesOut;
    input.m_buffers = &set;
    input.m_currentNodeIdx = job.m_currentNodeIdx;
    input.m_allowedEmptySplits.m_mask = hkVector4Comparison_s_maskFromBits[hkVector4Comparison::MASK_XYZ];
    input.m_startPrimIdx = job.m_startPrimIdx;
    input.m_endPrimIdx = job.m_endPrimIdx;
    input.m_bounds = job.m_bounds;
    input.m_maxNodeIdx = job.m_nodeArraySize;
    hkKdTreeBuilderSparseWriter writer;
    input.m_writer = &writer;
    int32_t numAvailableEmptyNodes = job.m_numAvailableEmptyNodes;
    treeOut.m_root = job.m_treeOut;
    alignas(16) int32_t numNodes = 1;
    *job.m_branchDepthOut =
        hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::buildTreeRecursive(
            input, &treeOut, job.m_projEntOut, numNodes, numAvailableEmptyNodes
        ) +
        2;
    const hkJobQueue::JobStatus status =
        jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    HK_TIMER_END();
    return status;
}

// Spreads the coordinator's collidables over up to four setup jobs (no more jobs than primitives) and queues them;
// the first one spawns the distributed build once all of them are done.
// KEEP: partcopy: of each 0x70-byte setup job nothing writes the bytes at +0x3, +0x8 to +0xF, +0x44 to +0x4F and
// +0x64 to +0x6F; initializeBuildSetupJobs (0xE2AA70) copies job 0 whole into the others (rep movsd, 0x1C dwords) and
// addJobQueueLocked (0xD04B00) copies a whole 0x80-byte queue entry per job into the queue.
hkJobQueue::JobStatus hkCpuKdTreeBuildCoordinatorJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeBuildCoord", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpKdTreeBuildCoordinatorJob& job =
        static_cast<const hkpKdTreeBuildCoordinatorJob&>(static_cast<const hkJob&>(nextJobOut));
    int32_t numJobs = job.m_numPrimitives;
    hkpKdTreeBuildSetupJob setupJobs[4];
    for (int32_t i = 0; i < 4; i++) {
        hkpKdTreeBuildSetupJob& setupJob = setupJobs[i];
        // EXE-INLINED none: hkpKdTreeBuildSetupJob::hkpKdTreeBuildSetupJob
        setupJob.m_jobSubType = static_cast<uint8_t>(hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_SETUP_JOB);
        setupJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_RAYCAST_QUERY);
        setupJob.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
        setupJob.m_size = static_cast<uint16_t>(sizeof(hkpKdTreeBuildSetupJob));
        setupJob.m_threadAffinity = -1;
        setupJob.m_semaphore = nullptr;
        setupJob.m_jobDoneFlag = nullptr;
        setupJob.m_sharedJobHeaderOnPpu = nullptr;
        setupJob.m_branchDepthsOut = nullptr;
        setupJob.m_spawnBuildDistributedJob = 0;
    }
    if (numJobs > 4) {
        numJobs = 4;
    }
    job.m_sharedJobHeaderOnPpu->m_openJobs = numJobs;
    if (numJobs > 0) {
        // The first setup job carries the distributed build's data.
        hkpKdTreeBuildSetupJob& firstJob = setupJobs[0];
        firstJob.m_semaphore = job.m_semaphore;
        firstJob.m_sharedJobHeaderOnPpu = job.m_sharedJobHeaderOnPpu;
        firstJob.m_collisionInput = job.m_collisionInput;
        firstJob.m_primitivesIn = job.m_primitivesIn;
        firstJob.m_primitivesOut = job.m_primitivesOut;
        firstJob.m_treeOut = job.m_treeOut;
        firstJob.m_projEntOut = job.m_projEntOut;
        firstJob.m_branchDepthsOut = job.m_branchDepthsOut;
        firstJob.m_subJobSizes = job.m_subJobSizes;
        firstJob.m_numPrimitives = job.m_numPrimitives;
        firstJob.m_nodeArraySize = job.m_nodeArraySize;
        firstJob.m_numAvailableEmptyNodes = job.m_numAvailableEmptyNodes;
        firstJob.m_spawnBuildDistributedJob = 1;
        hkKdTreeRayCastJobUtil::initializeBuildSetupJobs(
            job.m_collidables, job.m_numPrimitives, job.m_primitivesIn, job.m_semaphore, numJobs, setupJobs
        );

        // The SDK's storage for the queue's locked data.
        alignas(16) char dynamicDataStorage[sizeof(hkJobQueue::DynamicData)];
        hkJobQueue::DynamicData* data = jobQueue.lockQueue(dynamicDataStorage);
        for (int32_t i = 0; i < numJobs; i++) {
            // The queue takes the job as an entry: its hkJob base is the entry's.
            jobQueue.addJobQueueLocked(
                data, static_cast<const hkJobQueue::JobQueueEntry&>(static_cast<const hkJob&>(setupJobs[i])),
                hkJobQueue::JOB_HIGH_PRIORITY
            );
        }
        jobQueue.unlockQueue(data);
    }
    const hkJobQueue::JobStatus status =
        jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    HK_TIMER_END();
    return status;
}

// Projects the job's collidables [m_startIdx, m_endIdx) into its entries through a collidable mediator over the
// job's collidables, which the array does not own.
hkJobQueue::JobStatus hkCpuKdTreeBuildSetupJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CpuKdTreeBuildSetup", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpKdTreeBuildSetupJob& job =
        static_cast<const hkpKdTreeBuildSetupJob&>(static_cast<const hkJob&>(nextJobOut));
    hkArray<const hkpCollidable*, hkContainerHeapAllocator> collidables(job.m_collidables, job.m_endIdx, job.m_endIdx);
    hkJobQueue::JobStatus status;
    // The mediator is destroyed once the next job is taken, before the array.
    {
        hkpCollidableMediator mediator(collidables, nullptr);
        hkKdTreeBuildingUtils::projectPrimitives(&mediator, job.m_startIdx, job.m_endIdx, job.m_entriesOut);
        status = jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    }
    HK_TIMER_END();
    return status;
}
