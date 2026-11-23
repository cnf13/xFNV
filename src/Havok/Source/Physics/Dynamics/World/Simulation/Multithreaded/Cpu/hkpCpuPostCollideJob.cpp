#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuPostCollideJob.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommand.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandMachine.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

#include <xmmintrin.h>

// One island's post-collide work on a worker thread: it locks the island, merges the constraint info of every task into
// the island's own, runs the physics commands the tasks queued (prefetching the task four ahead, and timing each block
// of commands in the monitor stream), frees the tasks and unlocks the island. The thread then takes the next job; the
// result 2 is the "no queue" status returned for a null queue.
hkJobQueue::JobStatus
hkCpuPostCollideJob(hkpMtThreadStructure& tl, hkJobQueue* jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN_LIST("NarrowPhase", "PostCollide");
    // The queue entry holds the post-collide job in place.
    hkpPostCollideJob& job = reinterpret_cast<hkpPostCollideJob&>(nextJobOut);
    hkpSimulationIsland* pIsland = job.m_island;
    tl.m_world->lockIslandForConstraintUpdate(pIsland);
    hkpAgentSectorHeader* pHeader = job.m_header;
    const int32_t iNumTasks = pHeader->m_numTotalTasks;
    for (int32_t i = 0; i < iNumTasks; ++i) {
        if (i < iNumTasks - 4) {
            _mm_prefetch(
                reinterpret_cast<const char*>(pHeader->getJobInfos()[static_cast<int16_t>(i + 4)]),
                _MM_HINT_NTA
            );
        }
        hkpAgentSectorHeader::JobInfo* pTask = pHeader->getJobInfos()[static_cast<int16_t>(i)];
        int32_t iMaxSizeOfSchema = pTask->m_constraintInfo.m_maxSizeOfSchema;
        if (pIsland->m_constraintInfo.m_maxSizeOfSchema > iMaxSizeOfSchema) {
            iMaxSizeOfSchema = pIsland->m_constraintInfo.m_maxSizeOfSchema;
        }
        pIsland->m_constraintInfo.m_maxSizeOfSchema = iMaxSizeOfSchema;
        pIsland->m_constraintInfo.m_sizeOfSchemas += pTask->m_constraintInfo.m_sizeOfSchemas;
        pIsland->m_constraintInfo.m_numSolverResults += pTask->m_constraintInfo.m_numSolverResults;
        pIsland->m_constraintInfo.m_numSolverElemTemps += pTask->m_constraintInfo.m_numSolverElemTemps;
        const int32_t iNumCommandBytes = pTask->m_commandQueue.m_size;
        if (iNumCommandBytes != 0) {
            const int32_t iNumCommands = iNumCommandBytes >> 4;
            HK_MONITOR_ADD_VALUE("numCmds", static_cast<float>(iNumCommands), HK_MONITOR_TYPE_INT);
            // The queue's start is the byte distance from the queue to its first command (hkpRelativePointer32).
            hkPhysicsCommandMachineProcess(
                tl.m_world,
                reinterpret_cast<hkpPhysicsCommand*>(
                    reinterpret_cast<uint8_t*>(&pTask->m_commandQueue) + pTask->m_commandQueue.m_start.m_relativeAddress
                ),
                iNumCommandBytes
            );
        }
    }
    pHeader->deallocate();
    tl.m_world->unlockIslandForConstraintUpdate(pIsland);
    HK_TIMER_END_LIST();
    if (jobQueue != nullptr) {
        return jobQueue->finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    }
    return hkJobQueue::ALL_JOBS_FINISHED;
}

// Frees every task block the header lists and then the header.
void hkpAgentSectorHeader::deallocate() {
    for (int32_t i = 0; i < m_numTotalTasks; ++i) {
        int32_t iSizeOfJobInfo = this->m_sizeOfJobInfo;
        hkpAgentSectorHeader::JobInfo* v4 = getJobInfos()[i];
        hkMemoryRouter::getInstance().m_heap->blockFree(v4, iSizeOfJobInfo);
    }
    int32_t v6 = 4 * static_cast<int16_t>(m_numTotalTasks) + 12;
    hkMemoryRouter::getInstance().m_heap->blockFree(this, v6);
}

// Allocates the header with room for numTasks task pointers (sized with the count's low 16 bits), then one block per
// task: its JobInfo followed by room for numAgentNnEntriesPerTask commands, whose queue starts right after the JobInfo,
// empty. The queue's setup is skipped for a failed allocation.
hkpAgentSectorHeader* hkpAgentSectorHeader::allocate(int32_t numTasks, int32_t numAgentNnEntriesPerTask) {
    hkpAgentSectorHeader* pHeader = static_cast<hkpAgentSectorHeader*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(
        static_cast<int32_t>(sizeof(JobInfo*)) * static_cast<int16_t>(numTasks) +
        static_cast<int32_t>(sizeof(hkpAgentSectorHeader))
    ));
    const int32_t iCommandBytes = numAgentNnEntriesPerTask * hkpPhysicsCommandQueue::BYTES_PER_COMMAND;
    pHeader->m_openJobs = numTasks;
    pHeader->m_numTotalTasks = numTasks;
    pHeader->m_sizeOfJobInfo = iCommandBytes + static_cast<int32_t>(sizeof(JobInfo));
    for (int32_t i = 0; i < numTasks; ++i) {
        JobInfo* pInfo = static_cast<JobInfo*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(iCommandBytes + static_cast<int32_t>(sizeof(JobInfo)))
        );
        pHeader->getJobInfos()[i] = pInfo;
        if (pInfo != nullptr) {
            pInfo->m_commandQueue.init(reinterpret_cast<hkpPhysicsCommand*>(pInfo + 1), iCommandBytes);
        }
    }
    return pHeader;
}
