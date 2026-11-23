#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpJobQueueUtils.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// The SDK's spawnSplitSimulationIslandJob: queues, at high priority, the job that splits the job's island when the
// task header asks for a split check.
// KEEP: partcopy: the 32-byte job is built in a 16-aligned local with only bytes +0..+2, +4..+7, +0x10..+0x11 and
// +0x14..+0x1F written, and addJobQueueLocked enqueues a whole 128-byte JobQueueEntry copied from it: the unwritten
// bytes +3, +8..+0xF and +0x12..+0x13, and the 96 bytes past the local, reach the queue.
void spawnSplitSimulationIslandJob(
    const hkpBuildJacobianTaskHeader& localTaskHeader,
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkpDynamicsJob* dynamicsJob
) {
    if (localTaskHeader.m_splitCheckRequested) {
        alignas(16) hkpSplitSimulationIslandJob splitJob(*dynamicsJob);
        // The SDK passes the job as the queue entry it is copied into (the entry derives from hkJob).
        const hkJob& baseJob = splitJob;
        queue.addJobQueueLocked(
            data,
            static_cast<const hkJobQueue::JobQueueEntry&>(baseJob),
            hkJobQueue::JOB_HIGH_PRIORITY
        );
    }
}

// The SDK's spawnBuildJacobiansJobs: the island's build-jacobians job, one per build task, from dynamicsJob's island
// and taskHeader's tasks.
void spawnBuildJacobiansJobs(
    hkpBuildJacobianTaskHeader& taskHeader,
    const hkpDynamicsJob& dynamicsJob,
    hkJobQueue::JobQueueEntryInput& newJobCreated
) {
    new (&newJobCreated.m_job) hkpBuildJacobiansJob(dynamicsJob, taskHeader.m_tasks.m_buildJacobianTasks, taskHeader);
    taskHeader.m_openJobs = taskHeader.m_tasks.m_numBuildJacobianTasks;
    newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
    newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
}

// The queue's pop callback for the integrate jobs: the job taken is a copy of the queue entry with its island looked
// up again in the world's active islands. A job covering more than one job's share (islands, entities, accumulators,
// tasks) hands out the first share and stays in the queue with the rest; the broad-phase job splits itself.
hkJobQueue::JobPopFuncResult hkpJobQueueUtils::popIntegrateJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntry& jobOut
) {
    hkString::memCpy16NonEmpty(&jobOut, &jobIn, sizeof(hkJobQueue::JobQueueEntry) >> 4);
    hkpDynamicsJob& dynamicsJobIn = static_cast<hkpDynamicsJob&>(static_cast<hkJob&>(jobIn));
    hkpDynamicsJob& dynamicsJobOut = static_cast<hkpDynamicsJob&>(static_cast<hkJob&>(jobOut));
    // Reads the thread structure from the entry and the island index from the copy.
    hkArray<hkpSimulationIsland*>& islands = dynamicsJobIn.m_mtThreadStructure->m_world->m_activeSimulationIslands;
    dynamicsJobOut.m_island = islands.m_data[dynamicsJobOut.m_islandIndex];
    switch (jobIn.m_jobSubType) {
        case hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE: {
            hkpIntegrateJob& in = static_cast<hkpIntegrateJob&>(dynamicsJobIn);
            hkpIntegrateJob& out = static_cast<hkpIntegrateJob&>(dynamicsJobOut);
            int32_t numIslands = in.m_numIslands;
            if (numIslands <= 1) {
                break;
            }
            ++in.m_islandIndex;
            in.m_numIslands = numIslands - 1;
            out.m_numIslands = 1;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_ACCUMULATORS: {
            hkpBuildAccumulatorsJob& in = static_cast<hkpBuildAccumulatorsJob&>(dynamicsJobIn);
            hkpBuildAccumulatorsJob& out = static_cast<hkpBuildAccumulatorsJob&>(dynamicsJobOut);
            uint16_t numEntities = in.m_numEntities;
            if (numEntities <= hkpBuildAccumulatorsJob::ACCUMULATORS_PER_JOB) {
                break;
            }
            in.m_numEntities = numEntities - hkpBuildAccumulatorsJob::ACCUMULATORS_PER_JOB;
            in.m_firstEntityIdx += hkpBuildAccumulatorsJob::ACCUMULATORS_PER_JOB;
            out.m_numEntities = hkpBuildAccumulatorsJob::ACCUMULATORS_PER_JOB;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_FIRE_JACOBIAN_SETUP_CALLBACK:
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINTS:
        case hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE: jobOut = jobIn; break;
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_JACOBIANS: {
            hkpBuildJacobiansJob& in = static_cast<hkpBuildJacobiansJob&>(dynamicsJobIn);
            if (in.m_buildJacobianTask->m_next == nullptr) {
                break;
            }
            in.m_buildJacobianTask = in.m_buildJacobianTask->m_next;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_EXPORT_RESULTS: {
            hkpSolveExportResultsJob& in = static_cast<hkpSolveExportResultsJob&>(dynamicsJobIn);
            if (in.m_solveConstraintBatchTask->m_next == nullptr) {
                break;
            }
            in.m_solveConstraintBatchTask = in.m_solveConstraintBatchTask->m_next;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_APPLY_GRAVITY: {
            hkpSolveApplyGravityJob& in = static_cast<hkpSolveApplyGravityJob&>(dynamicsJobIn);
            hkpSolveApplyGravityJob& out = static_cast<hkpSolveApplyGravityJob&>(dynamicsJobOut);
            hkpVelocityAccumulator* split = in.m_accumulators +
                hkpSolveApplyGravityJob::MAX_NUM_ACCUMULATORS_FOR_APPLY_GRAVITY_JOB;
            if (split >= in.m_accumulatorsEnd) {
                break;
            }
            in.m_accumulators = split;
            out.m_accumulatorsEnd = split;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES: {
            hkpSolveIntegrateVelocitiesJob& in = static_cast<hkpSolveIntegrateVelocitiesJob&>(dynamicsJobIn);
            hkpSolveIntegrateVelocitiesJob& out = static_cast<hkpSolveIntegrateVelocitiesJob&>(dynamicsJobOut);
            hkpVelocityAccumulator* split = in.m_accumulators +
                hkpSolveIntegrateVelocitiesJob::MAX_NUM_ACCUMULATORS_FOR_INTEGRATE_VELOCITIES_JOB;
            if (split >= in.m_accumulatorsEnd) {
                break;
            }
            in.m_accumulators = split;
            out.m_accumulatorsEnd = split;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH: {
            hkpSolveConstraintBatchJob& in = static_cast<hkpSolveConstraintBatchJob&>(dynamicsJobIn);
            hkpSolveConstraintBatchTask* task = in.m_solveConstraintBatchTask;
            if (task->m_isLastTaskInBatch.m_bool != 0) {
                break;
            }
            in.m_solveConstraintBatchTask = task->m_next;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION: {
            hkpIntegrateMotionJob& in = static_cast<hkpIntegrateMotionJob&>(dynamicsJobIn);
            hkpIntegrateMotionJob& out = static_cast<hkpIntegrateMotionJob&>(dynamicsJobOut);
            uint16_t numEntities = in.m_numEntities;
            if (numEntities > hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB) {
                in.m_numEntities = numEntities - hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB;
                in.m_firstEntityIdx += hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB;
                out.m_numEntities = hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB;
                return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
            }
            // The last share: the entry stays in the queue as the island's export job when it has a task to export.
            if (in.m_solveConstraintBatchTask == nullptr) {
                break;
            }
            in.m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_EXPORT_RESULTS);
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_BROADPHASE:
            return static_cast<hkpBroadPhaseJob&>(dynamicsJobIn)
                .popJobTask(islands, static_cast<hkpBroadPhaseJob&>(dynamicsJobOut));
        default: break;
    }
    return hkJobQueue::POP_QUEUE_ENTRY;
}

// The queue's pop callback for the collide jobs: the job taken is a copy of the queue entry with its island looked up
// again in the world's active islands. An agent sector or agent entry job hands out m_numElementsPerTask elements and
// stays in the queue with the rest while it has more; a post-collide job is copied again and leaves the queue.
hkJobQueue::JobPopFuncResult hkpJobQueueUtils::popCollideJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntry& jobOut
) {
    hkString::memCpy16NonEmpty(&jobOut, &jobIn, sizeof(hkJobQueue::JobQueueEntry) >> 4);
    // The queue entry is the job's raw bytes (JobQueueEntry is untyped), so they are read as the job.
    hkpDynamicsJob& dynamicsJobIn = reinterpret_cast<hkpDynamicsJob&>(jobIn);
    hkpDynamicsJob& dynamicsJobOut = reinterpret_cast<hkpDynamicsJob&>(jobOut);
    // Reads the thread structure from the entry and the island index from the copy.
    dynamicsJobOut.m_island = dynamicsJobIn.m_mtThreadStructure->m_world->m_activeSimulationIslands
                                  .m_data[dynamicsJobOut.m_islandIndex];
    switch (jobIn.m_jobSubType) {
        case hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR: {
            hkpAgentSectorJob& in = reinterpret_cast<hkpAgentSectorJob&>(jobIn);
            hkpAgentSectorJob& out = reinterpret_cast<hkpAgentSectorJob&>(jobOut);
            uint16_t numElements = in.m_numElements;
            uint16_t numElementsPerTask = in.m_numElementsPerTask;
            if (numElements <= numElementsPerTask) {
                break;
            }
            ++in.m_taskIndex;
            in.m_numElements = numElements - numElementsPerTask;
            in.m_sectorIndex += numElementsPerTask;
            out.m_bytesUsedInLastSector = sizeof(hkpAgentNnSector);
            out.m_numElements = numElementsPerTask;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE: jobOut = jobIn; break;
        case hkpDynamicsJob::DYNAMICS_JOB_AGENT_NN_ENTRY: {
            hkpAgentNnEntryJob& in = reinterpret_cast<hkpAgentNnEntryJob&>(jobIn);
            hkpAgentNnEntryJob& out = reinterpret_cast<hkpAgentNnEntryJob&>(jobOut);
            uint16_t numElements = in.m_numElements;
            uint16_t numElementsPerTask = in.m_numElementsPerTask;
            if (numElements <= numElementsPerTask) {
                break;
            }
            ++in.m_taskIndex;
            in.m_numElements = numElements - numElementsPerTask;
            out.m_numElements = numElementsPerTask;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
        default: break;
    }
    return hkJobQueue::POP_QUEUE_ENTRY;
}

// The SDK's finish callback of the collide jobs: when the last agent sector job of an island finishes, it creates the
// island's post-collide job; no other collide job creates one.
hkJobQueue::JobCreationStatus hkpJobQueueUtils::finishCollideJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntryInput& newJobCreated
) {
    if (jobIn.m_jobSubType != hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR) {
        return hkJobQueue::NO_JOB_CREATED;
    }
    const hkpAgentSectorJob& job = static_cast<const hkpAgentSectorJob&>(static_cast<const hkJob&>(jobIn));
    hkpAgentSectorHeader* header = job.m_header;
    if (header == nullptr) {
        return hkJobQueue::NO_JOB_CREATED;
    }
    if (--header->m_openJobs != 0) {
        return hkJobQueue::NO_JOB_CREATED;
    }
    new (&newJobCreated.m_job) hkpPostCollideJob(job);
    newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
    return hkJobQueue::JOB_CREATED;
}

// The queue's finish callback for the integrate jobs: when the last open job of an island's stage finishes, it creates
// the job of the next stage (callbacks, Jacobians, gravity, constraint batches over the solver's micro steps and steps,
// velocities, motion, export, broad phase) and resets the task header's open-job count for it. After a build-jacobians
// stage with m_splitCheckRequested set, a split-island job is queued with some of its bytes never written, and the
// enqueue copies more bytes than the job holds into the queue.
hkJobQueue::JobCreationStatus hkpJobQueueUtils::finishIntegrateJob(
    hkJobQueue& queue,
    hkJobQueue::DynamicData* data,
    const hkJobQueue::JobQueueEntry& jobIn,
    hkJobQueue::JobQueueEntryInput& newJobCreated
) {
    const hkpDynamicsJob& dynamicsJob = static_cast<const hkpDynamicsJob&>(static_cast<const hkJob&>(jobIn));
    hkpBuildJacobianTaskHeader* taskHeader = dynamicsJob.m_taskHeader;
    // The source's placement new of each new job into the entry.
    hkJob* storage = &newJobCreated.m_job;
    switch (jobIn.m_jobSubType) {
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_ACCUMULATORS:
        case hkpDynamicsJob::DYNAMICS_JOB_CREATE_JACOBIAN_TASKS: {
            if (--taskHeader->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            if (taskHeader->m_tasks.m_numCallbackConstraints <= 0) {
                spawnBuildJacobiansJobs(*taskHeader, dynamicsJob, newJobCreated);
                return hkJobQueue::JOB_CREATED;
            }
            new (storage) hkpFireJacobianSetupCallback(static_cast<const hkpBuildAccumulatorsJob&>(dynamicsJob));
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_FIRE_JACOBIAN_SETUP_CALLBACK:
            spawnBuildJacobiansJobs(*taskHeader, dynamicsJob, newJobCreated);
            return hkJobQueue::JOB_CREATED;
        case hkpDynamicsJob::DYNAMICS_JOB_BUILD_JACOBIANS: {
            if (--taskHeader->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            // The job just created; the split-island job below copies its island fields.
            hkpDynamicsJob* createdJob = nullptr;
            if (taskHeader->m_solveInSingleThread.m_bool != 0) {
                createdJob = new (storage) hkpSolveConstraintsJob(dynamicsJob, *taskHeader);
                newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
                newJobCreated.m_job.m_jobSpuType = taskHeader->m_solveInSingleThreadOnPpuOnly.m_bool != 0
                    ? HK_JOB_SPU_TYPE_DISABLED
                    : HK_JOB_SPU_TYPE_ENABLED;
            }
            else {
                createdJob = new (storage) hkpSolveApplyGravityJob(dynamicsJob, *taskHeader);
                newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
                newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
                taskHeader->m_openJobs = taskHeader->m_numApplyGravityJobs;
            }
            spawnSplitSimulationIslandJob(*taskHeader, queue, data, createdJob);
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINTS:
            taskHeader->m_exportFinished = 1;
            return hkJobQueue::NO_JOB_CREATED;
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_APPLY_GRAVITY: {
            if (--taskHeader->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            new (storage)
                hkpSolveConstraintBatchJob(static_cast<const hkpSolveApplyGravityJob&>(dynamicsJob), *taskHeader);
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
            taskHeader->m_openJobs = taskHeader->m_solveTasks.m_firstBatchSize;
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH: {
            if (--taskHeader->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            const hkpSolveConstraintBatchJob& batchJob = static_cast<const hkpSolveConstraintBatchJob&>(dynamicsJob);
            int32_t numOpenJobs;
            uint16_t sizeOfNextBatch = batchJob.m_solveConstraintBatchTask->m_sizeOfNextBatch;
            if (sizeOfNextBatch != 0) {
                // The next batch of this micro step.
                new (storage) hkpSolveConstraintBatchJob(batchJob);
                numOpenJobs = sizeOfNextBatch;
            }
            else if (batchJob.m_currentSolverMicroStep + 1 < batchJob.m_numSolverMicroSteps) {
                // The first batch of the next micro step.
                hkpSolveConstraintBatchJob* job = new (storage) hkpSolveConstraintBatchJob(batchJob);
                ++job->m_currentSolverMicroStep;
                job->m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
                numOpenJobs = taskHeader->m_solveTasks.m_firstBatchSize;
            }
            else {
                // The step's micro steps are done: integrate the velocities.
                new (storage) hkpSolveIntegrateVelocitiesJob(batchJob, *taskHeader);
                numOpenJobs = taskHeader->m_numIntegrateVelocitiesJobs;
            }
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
            taskHeader->m_openJobs = numOpenJobs;
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES: {
            if (--taskHeader->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            const hkpSolveIntegrateVelocitiesJob& velocitiesJob = static_cast<const hkpSolveIntegrateVelocitiesJob&>(
                dynamicsJob
            );
            if (velocitiesJob.m_solvingFinished.m_bool == 0) {
                // The next solver step, from the first batch.
                new (storage) hkpSolveConstraintBatchJob(velocitiesJob, *taskHeader);
                newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
                newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
                taskHeader->m_openJobs = taskHeader->m_solveTasks.m_firstBatchSize;
                return hkJobQueue::JOB_CREATED;
            }
            // Solving is finished: integrate the motions.
            new (storage) hkpIntegrateMotionJob(velocitiesJob, *taskHeader);
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            newJobCreated.m_job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
            taskHeader->m_openJobs = taskHeader->m_tasks.m_numBuildJacobianTasks;
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_EXPORT_RESULTS: {
            taskHeader->m_openJobs--;
            const hkpSolveExportResultsJob& exportJob = static_cast<const hkpSolveExportResultsJob&>(dynamicsJob);
            // The task's schema buffer is raw memory that starts with its block of breached impulse limits; a block
            // with any is linked into the task header's list.
            hkpImpulseLimitBreachedHeader* breached = reinterpret_cast<hkpImpulseLimitBreachedHeader*>(
                exportJob.m_solveConstraintBatchTask->m_schemas
            );
            if (breached->m_numBreached != 0) {
                breached->m_next = taskHeader->m_impulseLimitsBreached;
                taskHeader->m_impulseLimitsBreached = breached;
            }
            if (taskHeader->m_openJobs == 0) {
                taskHeader->m_exportFinished = 1;
            }
            return hkJobQueue::NO_JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_SPLIT_ISLAND:
        case hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION: {
            // A split-island job counts as one finished job for the broad phase, a motion job as one per entity share.
            int32_t numFinishedJobs = 1;
            if (jobIn.m_jobSubType == hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION) {
                const hkpIntegrateMotionJob& motionJob = static_cast<const hkpIntegrateMotionJob&>(dynamicsJob);
                if (motionJob.m_numInactiveFrames <= 5) {
                    taskHeader->m_islandShouldBeDeactivated = 0;
                }
                numFinishedJobs = static_cast<int32_t>(
                    static_cast<uint32_t>(motionJob.m_numEntities - 1) / hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB + 1
                );
            }
            dynamicsJob.m_taskHeader->m_numUnfinishedJobsForBroadphase -= numFinishedJobs;
            if (dynamicsJob.m_taskHeader->m_numUnfinishedJobsForBroadphase != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            new (storage) hkpBroadPhaseJob(dynamicsJob, dynamicsJob.m_taskHeader);
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            return hkJobQueue::JOB_CREATED;
        }
        case hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR: {
            const hkpAgentSectorJob& sectorJob = static_cast<const hkpAgentSectorJob&>(dynamicsJob);
            hkpAgentSectorHeader* header = sectorJob.m_header;
            if (header == nullptr) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            if (--header->m_openJobs != 0) {
                return hkJobQueue::NO_JOB_CREATED;
            }
            new (storage) hkpPostCollideJob(sectorJob);
            newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
            return hkJobQueue::JOB_CREATED;
        }
        default: return hkJobQueue::NO_JOB_CREATED;
    }
}
