#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobQueueUtils.h"

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Physics/Collide/Gjk/Penetration/hkpConvexPenetrationUtil.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

// Copies the distributed k-d tree build fields of apFrom into apTo, all but the hkJob header and m_jobDoneFlag. Its one
// caller, hkpRayCastQueryJobQueueUtils::finishRayCastQueryJob, passes the finished setup job and the distributed job it
// creates.
static int32_t
copyKdTreeBuildDistributedJob(const hkpKdTreeBuildDistributedJob* apFrom, hkpKdTreeBuildDistributedJob* apTo);

static int32_t
copyKdTreeBuildDistributedJob(const hkpKdTreeBuildDistributedJob* apFrom, hkpKdTreeBuildDistributedJob* apTo) {
    apTo->m_sharedJobHeaderOnPpu = apFrom->m_sharedJobHeaderOnPpu;
    apTo->m_collisionInput = apFrom->m_collisionInput;
    apTo->m_semaphore = apFrom->m_semaphore;
    apTo->m_primitivesIn = apFrom->m_primitivesIn;
    apTo->m_primitivesOut = apFrom->m_primitivesOut;
    apTo->m_numPrimitives = apFrom->m_numPrimitives;
    apTo->m_treeOut = apFrom->m_treeOut;
    apTo->m_projEntOut = apFrom->m_projEntOut;
    apTo->m_branchDepthsOut = apFrom->m_branchDepthsOut;
    apTo->m_nodeArraySize = apFrom->m_nodeArraySize;
    apTo->m_subJobSizes = apFrom->m_subJobSizes;
    const int32_t iResult = apFrom->m_numAvailableEmptyNodes;
    apTo->m_numAvailableEmptyNodes = iResult;
    return iResult;
}

// Does nothing on the PC.
void hkpRayCastQueryJobQueueUtils::addKdTreeJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn
) {
    ;
}

// The queue's pop callback for the ray cast query jobs: it copies the queue entry into the job the queue takes out and,
// for the shape and world ray cast jobs, hands out at most m_numCommandsPerTask commands, leaving the rest of the entry
// in the queue. A world ray cast job left in the queue keeps no bundle commands: the task handed out takes them all.
// The entry holds a job of any type: both it and the ray cast jobs derive from hkJob, so the job is reached by a
// downcast from that base.
hkJobQueue::JobPopFuncResult hkpRayCastQueryJobQueueUtils::popRayCastQueryJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntry& jobOut
) {
    jobOut = jobIn;
    hkJob& jobInBase = jobIn;
    hkJob& jobOutBase = jobOut;
    switch (static_cast<hkpRayCastQueryJob::JobSubType>(jobInBase.m_jobSubType)) {
        case hkpRayCastQueryJob::RAYCAST_QUERY_SHAPE_RAYCAST: {
            hkpShapeRayCastJob& in = static_cast<hkpShapeRayCastJob&>(jobInBase);
            hkpShapeRayCastJob& out = static_cast<hkpShapeRayCastJob&>(jobOutBase);
            if (in.m_numCommands <= in.m_numCommandsPerTask) {
                break;
            }
            out.m_numCommands = in.m_numCommandsPerTask;
            in.m_numCommands -= in.m_numCommandsPerTask;
            in.m_commandArray += in.m_numCommandsPerTask;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpRayCastQueryJob::RAYCAST_QUERY_WORLD_RAYCAST: {
            hkpWorldRayCastJob& in = static_cast<hkpWorldRayCastJob&>(jobInBase);
            hkpWorldRayCastJob& out = static_cast<hkpWorldRayCastJob&>(jobOutBase);
            if (in.m_numCommands <= in.m_numCommandsPerTask) {
                break;
            }
            out.m_numCommands = in.m_numCommandsPerTask;
            in.m_numCommands -= in.m_numCommandsPerTask;
            in.m_commandArray += in.m_numCommandsPerTask;
            in.m_numBundleCommands = 0;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        default: break;
    }
    return hkJobQueue::POP_QUEUE_ENTRY;
}

// The queue's finish callback for the ray cast query jobs. A finished shape or world ray cast job or k-d tree sub job
// counts its header's open jobs down (a plain decrement) and the last one releases the waiting
// thread; a setup job that is to spawn the distributed build does so once it is the last open one; an auto-bundle job
// hands its single and bundled commands on as a world ray cast job. The new jobs are placement-constructed into
// newJobCreated.m_job.
hkJobQueue::JobCreationStatus hkpRayCastQueryJobQueueUtils::finishRayCastQueryJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntryInput& newJobCreated
) {
    const hkJob& jobInBase = jobIn;
    const hkpRayCastQueryJob& job = static_cast<const hkpRayCastQueryJob&>(jobInBase);
    hkpCollisionQueryJobHeader* pHeader = job.m_sharedJobHeaderOnPpu;
    switch (static_cast<hkpRayCastQueryJob::JobSubType>(jobInBase.m_jobSubType)) {
        case hkpRayCastQueryJob::RAYCAST_QUERY_SHAPE_RAYCAST:
        case hkpRayCastQueryJob::RAYCAST_QUERY_WORLD_RAYCAST:
        case hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_FAST_DISTRIBUTED_SUBJOB:
            if (--pHeader->m_openJobs == 0) {
                job.atomicIncrementAndReleaseSemaphore();
            }
            break;
        case hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_FAST: job.atomicIncrementAndReleaseSemaphore(); break;
        case hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_FAST_DISTRIBUTED_JOB: {
            const hkpKdTreeBuildDistributedJob& distributedJob = static_cast<const hkpKdTreeBuildDistributedJob&>(job);
            if (distributedJob.m_numPrimitives < 8) {
                job.atomicIncrementAndReleaseSemaphore();
            }
            break;
        }
        case hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_SETUP_JOB: {
            const hkpKdTreeBuildSetupJob& setupJob = static_cast<const hkpKdTreeBuildSetupJob&>(job);
            if (pHeader) {
                --pHeader->m_openJobs;
            }
            if (!setupJob.m_spawnBuildDistributedJob) {
                job.atomicIncrementAndReleaseSemaphore();
                break;
            }
            if (pHeader->m_openJobs != 0) {
                break;
            }
            hkpKdTreeBuildDistributedJob* pNew = ::new (&newJobCreated.m_job)
                hkpKdTreeBuildDistributedJob(setupJob.m_sharedJobHeaderOnPpu);
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            copyKdTreeBuildDistributedJob(&setupJob, pNew);
            return hkJobQueue::JOB_CREATED;
        }
        case hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_COORDINATOR_JOB:
            if (pHeader->m_openJobs == 0) {
                job.atomicIncrementAndReleaseSemaphore();
            }
            break;
        case hkpRayCastQueryJob::RAYCAST_QUERY_WORLD_RAYCAST_WITH_AUTOBUNDLE: {
            const hkpWorldRayCastWithAutoBundleJob& autoJob = static_cast<const hkpWorldRayCastWithAutoBundleJob&>(job);
            hkpWorldRayCastJob* pNew = ::new (&newJobCreated.m_job) hkpWorldRayCastJob(
                autoJob.m_collisionInput,
                autoJob.m_jobHeader,
                autoJob.m_singleCommands,
                autoJob.m_numSingleCommands,
                autoJob.m_broadPhase,
                autoJob.m_semaphore
            );
            pNew->m_aabbTreeFp = autoJob.m_aabbTreeFp;
            pNew->m_aabbTreeInt16 = autoJob.m_aabbTreeInt16;
            pNew->m_kdTree = autoJob.m_kdTree;
            pNew->m_bundleCommandArray = autoJob.m_bundledCommands;
            pNew->m_numBundleCommands = autoJob.m_numBundledCommands;
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_LOW_PRIORITY;
            newJobCreated.m_job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
            return hkJobQueue::JOB_CREATED;
        }
        default: break;
    }
    return hkJobQueue::NO_JOB_CREATED;
}

void hkpRayCastQueryJob::atomicIncrementAndReleaseSemaphore() const {
    int32_t* jobDoneFlag = reinterpret_cast<int32_t*>(m_jobDoneFlag);
    if (jobDoneFlag) {
        InterlockedExchangeAdd(jobDoneFlag, 1);
    }
    hkSemaphore* semaphore = m_semaphore;
    if (semaphore) {
        hkSemaphore::release(semaphore, 1);
    }
}

void hkpConvexPenetrationUtil::codeBegin() {}
