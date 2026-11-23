#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

class hkpJacobianSchema;
struct hkpMtThreadStructure;
class hkpVelocityAccumulator;

// What hkCpuBuildJacobiansFromTask builds from: a task's atom infos, accumulators and schema range.
struct hkBuildJacobiansFromTaskInput {
    hkpBuildJacobianTask::AtomInfo* m_atomInfos;
    int32_t m_numAtomInfos;
    const hkpVelocityAccumulator* m_accumulators;
    hkpJacobianSchema* m_schemas;
    hkpJacobianSchema* m_schemasOfNextTask;
    void* m_nextTask;
    hkBool m_finishSchemasWithGoto;
};

// Builds the Jacobians of one island's constraint batch and takes the next job.
hkJobQueue::JobStatus
hkCpuBuildJacobiansJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
// File-local to hkpCpuBuildJacobiansJob.cpp. Per atom info it sets queryIn's bodies, transforms and constraint data and
// builds the Jacobians.
void hkCpuBuildJacobiansFromTask(const hkBuildJacobiansFromTaskInput& input, hkpConstraintQueryIn& queryIn);
