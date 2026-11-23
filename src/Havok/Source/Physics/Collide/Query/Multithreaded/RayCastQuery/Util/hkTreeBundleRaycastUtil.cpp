#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkTreeBundleRaycastUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuAutoRayBundleJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

// KEEP (partcopy:): addJob copies each job whole (m_size bytes) into the queue, and nothing writes the job's
// padding03, the padding at the end of its hkJob base, the base's m_collisionInput, m_numSingleCommands or
// m_numBundledCommands; the octant-list jobs never write m_octantIdx either (an uninitialised read). With
// scanAllInJobs, eight jobs scan all rays, one octant each; else the rays are sorted into the eight octants on this
// thread and each non-empty octant's list becomes a job. The arrays' constructors and destructors are inlined (their
// sizes are never stored); the two 8-int arrays are allocated and freed unused.
void hkTreeBundleRayCastUtil::startWorldRayCastWithAutoBundleJobs(
    const hkVector4* rayFrom,
    const hkVector4* rayTo,
    int32_t numRays,
    float angleVariance,
    float distanceVariance2,
    hkpWorldRayCastJob* worldRayCastJob,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool,
    hkpWorldRayCastOutput* resultBuffer,
    bool scanAllInJobs
) {
    if (scanAllInJobs) {
        HK_TIMER_BEGIN_LIST("AutoJob", "AddJobs");
        // The job holds the world job's const commands as non-const pointers.
        hkpWorldRayCastCommand* singleCommands = const_cast<hkpWorldRayCastCommand*>(worldRayCastJob->m_commandArray);
        hkpWorldRayCastBundleCommand* bundledCommands = const_cast<hkpWorldRayCastBundleCommand*>(
            worldRayCastJob->m_bundleCommandArray
        );
        for (int32_t octant = 0; octant < 8; ++octant) {
            hkpWorldRayCastWithAutoBundleJob job;
            job.m_angleVariance = angleVariance;
            job.m_distanceVariance2 = distanceVariance2;
            job.m_numRays = numRays;
            job.m_rayFrom = rayFrom;
            job.m_rayTo = rayTo;
            job.m_singleCommands = singleCommands;
            job.m_resultBuffer = resultBuffer;
            job.m_bundledCommands = bundledCommands;
            job.m_jobHeader = worldRayCastJob->m_sharedJobHeaderOnPpu;
            job.m_collisionInput = worldRayCastJob->m_collisionInput;
            job.m_semaphore = worldRayCastJob->m_semaphore;
            job.m_aabbTreeFp = worldRayCastJob->m_aabbTreeFp;
            job.m_broadPhase = worldRayCastJob->m_broadphase;
            job.m_aabbTreeInt16 = worldRayCastJob->m_aabbTreeInt16;
            job.m_rayIndices = nullptr;
            job.m_octantIdx = octant;
            job.m_numIndices = 0;
            job.m_kdTree = worldRayCastJob->m_kdTree;
            jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
            singleCommands += numRays;
            bundledCommands += numRays;
        }
        jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
        HK_TIMER_SPLIT_LIST("WaitThreads");
        jobThreadPool->waitForCompletion();
        HK_TIMER_END_LIST();
        worldRayCastJob->m_semaphore->acquire();
        return;
    }

    HK_TIMER_BEGIN("CPUSplit", nullptr);
    hkArray<hkVector4> directions(numRays);
    hkArray<hkVector4> lengthsSquared(numRays);
    hkArray<hkArray<int32_t>> octantRays(8);
    alignas(16) int32_t octantCounts[8];
    hkpAutoRayBundleJob_sortRaysByOctant(
        rayFrom,
        rayTo,
        numRays,
        directions.m_data,
        lengthsSquared.m_data,
        octantRays.m_data,
        octantCounts
    );
    hkArray<int32_t> unusedA(8);
    hkArray<int32_t> unusedB(8);

    // The job holds the world job's const commands as non-const pointers.
    hkpWorldRayCastCommand* singleCommands = const_cast<hkpWorldRayCastCommand*>(worldRayCastJob->m_commandArray);
    hkpWorldRayCastBundleCommand* bundledCommands = const_cast<hkpWorldRayCastBundleCommand*>(
        worldRayCastJob->m_bundleCommandArray
    );
    hkArray<int32_t>* octant = octantRays.m_data;
    for (int32_t remaining = 8; remaining != 0; --remaining, ++octant) {
        if (!octant->m_size) {
            continue;
        }
        hkpWorldRayCastWithAutoBundleJob job;
        job.m_angleVariance = angleVariance;
        job.m_distanceVariance2 = distanceVariance2;
        job.m_rayFrom = rayFrom;
        job.m_rayIndices = octant->m_data;
        job.m_numIndices = octant->m_size;
        job.m_singleCommands = singleCommands;
        job.m_rayTo = rayTo;
        job.m_resultBuffer = resultBuffer;
        job.m_jobHeader = worldRayCastJob->m_sharedJobHeaderOnPpu;
        job.m_bundledCommands = bundledCommands;
        job.m_collisionInput = worldRayCastJob->m_collisionInput;
        job.m_aabbTreeFp = worldRayCastJob->m_aabbTreeFp;
        job.m_semaphore = worldRayCastJob->m_semaphore;
        job.m_broadPhase = worldRayCastJob->m_broadphase;
        job.m_numRays = numRays;
        job.m_aabbTreeInt16 = worldRayCastJob->m_aabbTreeInt16;
        job.m_kdTree = worldRayCastJob->m_kdTree;
        jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
        singleCommands += numRays;
        bundledCommands += numRays;
    }
    HK_TIMER_END();

    HK_TIMER_BEGIN("AutoBundleJobs", nullptr);
    jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    jobThreadPool->waitForCompletion();
    worldRayCastJob->m_semaphore->acquire();
    HK_TIMER_END();
}
