#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuAgentSectorJob.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnEntry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandMachine.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

#include <optional>
#include <xmmintrin.h>

// The agents of the job's sectors (an agent sector job) or of its entries (an agent entry job, for TOI) are processed
// with a copy of the thread's collision input and a deferred constraint owner, which turns the constraint additions and
// removals of each agent into queued commands. The constraint info and the commands go to the job's task block, or for
// a sector job without a header straight to the island under its constraint lock. The queued commands carry bytes
// nothing wrote, as the game does: each command's type is a byte stored into a 32-bit value whose upper three bytes
// nothing writes, and each 16-byte command slot of the queue's buffer is written only partly by an add or a remove; the
// slots are copied whole into the task block or run by hkPhysicsCommandMachineProcess. The input's destructor (the
// spare agent sector's free) runs after the next job is taken.
hkJobQueue::JobStatus
hkCpuAgentSectorJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    // The queue entry holds the job: an hkJob whose bytes after the base are the dynamics job's.
    const hkpAgentBaseJob& job = static_cast<const hkpAgentBaseJob&>(static_cast<const hkJob&>(nextJobOut));

    alignas(16) hkpProcessCollisionInput input(tl.m_collisionInput);
    input.m_stepInfo = job.m_stepInfo;

    hkpDeferredConstraintOwner constraintOwner;
    // EXE-INLINED none: hkpDeferredConstraintOwner::hkpDeferredConstraintOwner
    constraintOwner.m_constraintInfo.clear();
    constraintOwner.m_constraintAddRemoveCounter.m_storage = 0;
    constraintOwner.m_callbackRequestForAddConstraint.m_storage = 0;

    // EXE-INLINED none: hkpProcessCollisionOutput::hkpProcessCollisionOutput
    hkpProcessCollisionOutput processOutput;
    processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
    processOutput.m_toi.m_properties.m_impulseApplied = 0.0f;
    processOutput.m_toi.m_properties.m_internalSolverData = 0.0f;
    processOutput.m_constraintOwner.m_storage = &constraintOwner;

    if (job.m_jobSubType == hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR) {
        const hkpAgentSectorJob& sectorJob = static_cast<const hkpAgentSectorJob&>(job);
        hkpSimulationIsland* island = sectorJob.m_island;
        HK_TIMER_BEGIN("NarrowPhase", nullptr);
        // A sector is raw untyped bytes: its entries sit HK_AGENT3_AGENT_SIZE bytes apart. Two entries ahead are kept;
        // the one after the next steps into the following sector, whose successor is prefetched (its address as the
        // intrinsic's raw byte pointer).
        int32_t sectorsLeft = sectorJob.m_numElements - 1;
        int32_t sectorIndex = sectorJob.m_sectorIndex;
        hkpAgentNnEntry* entry = island->m_agentTrack.m_sectors.m_data[sectorIndex]->getBegin();
        const uint32_t bytesInFirstSector = (sectorIndex + 1 == island->m_agentTrack.m_sectors.m_size)
            ? island->m_agentTrack.m_bytesUsedInLastSector
            : HK_AGENT3_SECTOR_SIZE;
        hkpAgentNnEntry* sectorEnd =
            reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + bytesInFirstSector);
        hkpAgentNnEntry* next =
            reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(entry) + HK_AGENT3_AGENT_SIZE);
        hkpAgentNnEntry* nextNext =
            reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(next) + HK_AGENT3_AGENT_SIZE);
        if (next >= sectorEnd) {
            next = nullptr;
            nextNext = nullptr;
        }
        else if (nextNext >= sectorEnd) {
            nextNext = nullptr;
        }
        do {
            hkpMultiThreadedSimulation* simulation = tl.m_simulation.m_storage;
            hkpLinkedCollidable* collA = entry->m_collidable[0];
            hkpLinkedCollidable* collB = entry->m_collidable[1];
            hkpCollisionQualityInfo* qualityInfo =
                input.m_dispatcher->getCollisionQualityInfo(static_cast<int8_t>(entry->m_collisionQualityIndex));
            input.m_collisionQualityInfo = qualityInfo;
            input.m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics.m_storage;
            // EXE-INLINED none: hkpProcessCollisionOutput::reset
            processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
            processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
            processOutput.m_potentialContacts.m_storage = nullptr;
            hkAgentNnMachine_ProcessAgent(entry, input, processOutput, entry->m_contactMgr);
            if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                entry->m_contactMgr->processContact(*collA, *collB, input, processOutput);
            }
            if (processOutput.m_toi.m_time.m_storage != 3.402820018375656e+38) {
                simulation->addToiEventWithCriticalSectionLock(
                    processOutput, *entry, &simulation->m_toiQueueCriticalSection
                );
            }
            // The constraint the agent added or removed becomes a queued command.
            if (constraintOwner.m_constraintAddRemoveCounter.m_storage != 0) {
                if (constraintOwner.m_constraintAddRemoveCounter.m_storage > 0) {
                    // EXE-INLINED none: hkpAddConstraintToCriticalLockedIslandPhysicsCommand's constructor
                    hkpAddConstraintToCriticalLockedIslandPhysicsCommand command;
                    command.m_type = hkpPhysicsCommand::TYPE_ADD_CONSTRAINT_TO_LOCKED_ISLAND;
                    command.m_object0 = constraintOwner.m_constraintForCommand;
                    command.m_object1 = constraintOwner.m_callbackRequestForAddConstraint.m_storage;
                    constraintOwner.m_commandQueue.addCommand(command);
                    constraintOwner.m_callbackRequestForAddConstraint.m_storage = 0;
                }
                else {
                    // EXE-INLINED none: hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand's constructor
                    hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand command;
                    command.m_type = hkpPhysicsCommand::TYPE_REMOVE_CONSTRAINT_FROM_LOCKED_ISLAND;
                    command.m_object = constraintOwner.m_constraintForCommand;
                    constraintOwner.m_commandQueue.addCommand(command);
                }
            }
            else {
                constraintOwner.m_callbackRequestForAddConstraint.m_storage = 0;
            }
            constraintOwner.m_constraintAddRemoveCounter.m_storage = 0;

            entry = next;
            next = nextNext;
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                break;
            }
            if (nextNext) {
                nextNext =
                    reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(nextNext) + HK_AGENT3_AGENT_SIZE);
                if (nextNext >= sectorEnd) {
                    ++sectorIndex;
                    if (--sectorsLeft < 0) {
                        nextNext = nullptr;
                    }
                    else {
                        hkpAgentNnSector** sectors = &island->m_agentTrack.m_sectors.m_data[sectorIndex];
                        nextNext = sectors[0]->getBegin();
                        if (sectorsLeft > 0) {
                            const char* prefetch = reinterpret_cast<const char*>(sectors[1]);
                            _mm_prefetch(prefetch, _MM_HINT_NTA);
                            _mm_prefetch(prefetch + 0x40, _MM_HINT_NTA);
                            _mm_prefetch(prefetch + 0x80, _MM_HINT_NTA);
                            _mm_prefetch(prefetch + 0xC0, _MM_HINT_NTA);
                        }
                        const uint32_t bytesInSector = (sectorIndex + 1 == island->m_agentTrack.m_sectors.m_size)
                            ? island->m_agentTrack.m_bytesUsedInLastSector
                            : HK_AGENT3_SECTOR_SIZE;
                        sectorEnd =
                            reinterpret_cast<hkpAgentNnEntry*>(reinterpret_cast<uint8_t*>(nextNext) + bytesInSector);
                    }
                }
            }
        }
        while (entry);

        if (job.m_header) {
            hkpAgentSectorHeader::JobInfo* jobInfo = job.m_header->getJobInfos()[static_cast<int16_t>(job.m_taskIndex)];
            jobInfo->m_constraintInfo = constraintOwner.m_constraintInfo;
            const int32_t size = static_cast<int32_t>(constraintOwner.m_commandQueue.m_size);
            if (size) {
                // EXE-INLINED none: hkString::memCpy16 (the queues' buffers are untyped command bytes)
                uint32_t* dst32 = reinterpret_cast<uint32_t*>(jobInfo->m_commandQueue.m_start.val());
                const uint32_t* src32 = reinterpret_cast<const uint32_t*>(constraintOwner.m_commandQueue.m_start.val());
                for (int32_t numQuads = size >> 4; numQuads > 0; --numQuads) {
                    dst32[0] = src32[0];
                    dst32[1] = src32[1];
                    dst32[2] = src32[2];
                    dst32[3] = src32[3];
                    dst32 += 4;
                    src32 += 4;
                }
                jobInfo->m_commandQueue.m_size = static_cast<uint32_t>(size);
            }
        }
        else if (
            (constraintOwner.m_constraintInfo.m_numSolverElemTemps |
             constraintOwner.m_constraintInfo.m_numSolverResults | constraintOwner.m_constraintInfo.m_sizeOfSchemas |
             constraintOwner.m_constraintInfo.m_maxSizeOfSchema) != 0 ||
            constraintOwner.m_commandQueue.m_size != 0
        ) {
            tl.m_world->lockIslandForConstraintUpdate(island);
            // EXE-INLINED none: hkpConstraintInfo::merge
            int32_t maxSizeOfSchema = island->m_constraintInfo.m_maxSizeOfSchema;
            if (maxSizeOfSchema <= constraintOwner.m_constraintInfo.m_maxSizeOfSchema) {
                maxSizeOfSchema = constraintOwner.m_constraintInfo.m_maxSizeOfSchema;
            }
            island->m_constraintInfo.m_maxSizeOfSchema = maxSizeOfSchema;
            island->m_constraintInfo.m_sizeOfSchemas += constraintOwner.m_constraintInfo.m_sizeOfSchemas;
            island->m_constraintInfo.m_numSolverResults += constraintOwner.m_constraintInfo.m_numSolverResults;
            island->m_constraintInfo.m_numSolverElemTemps += constraintOwner.m_constraintInfo.m_numSolverElemTemps;
            if (constraintOwner.m_commandQueue.m_size) {
                hkPhysicsCommandMachineProcess(
                    tl.m_world, constraintOwner.m_commandQueue.m_start.val(),
                    static_cast<int32_t>(constraintOwner.m_commandQueue.m_size)
                );
            }
            tl.m_world->unlockIslandForConstraintUpdate(island);
        }
        HK_TIMER_NAMED_END("NarrowPhase");
    }
    else {
        const hkpAgentNnEntryJob& entryJob = static_cast<const hkpAgentNnEntryJob&>(job);
        HK_TIMER_BEGIN("NarrowPhaseTOI", nullptr);
        const int32_t firstElement = entryJob.m_numElementsPerTask * entryJob.m_taskIndex;
        for (int32_t i = 0; i < entryJob.m_numElements; ++i) {
            hkpAgentNnEntry* entry = static_cast<hkpAgentNnEntry*>(entryJob.m_elements[firstElement + i]);
            if (entryJob.m_forceManifoldQualityType.m_bool) {
                input.m_collisionQualityInfo = input.m_dispatcher->getCollisionQualityInfo(
                    hkpCollisionDispatcher::COLLISION_QUALITY_TMP_EXPAND_MANIFOLD
                );
                input.m_createPredictiveAgents.m_storage =
                    input.m_dispatcher->getCollisionQualityInfo(static_cast<int8_t>(entry->m_collisionQualityIndex))
                        ->m_useContinuousPhysics.m_storage;
            }
            else {
                hkpCollisionQualityInfo* qualityInfo =
                    input.m_dispatcher->getCollisionQualityInfo(static_cast<int8_t>(entry->m_collisionQualityIndex));
                input.m_collisionQualityInfo = qualityInfo;
                input.m_createPredictiveAgents.m_storage = qualityInfo->m_useContinuousPhysics.m_storage;
            }
            // EXE-INLINED none: hkpProcessCollisionOutput::reset
            processOutput.m_toi.m_time.m_storage = 3.40282e+38f;
            processOutput.m_firstFreeContactPoint.m_storage = processOutput.m_contactPoints;
            processOutput.m_potentialContacts.m_storage = nullptr;
            hkAgentNnMachine_ProcessAgent(entry, input, processOutput, entry->m_contactMgr);
            if (processOutput.m_firstFreeContactPoint.m_storage != processOutput.m_contactPoints) {
                entry->m_contactMgr->processContact(
                    *entry->m_collidable[0], *entry->m_collidable[1], input, processOutput
                );
            }
            if (processOutput.m_toi.m_time.m_storage != 3.402820018375656e+38) {
                tl.m_simulation.m_storage->addToiEventWithCriticalSectionLock(
                    processOutput, *entry, &tl.m_simulation.m_storage->m_toiQueueCriticalSection
                );
            }
            // The constraint the agent added or removed becomes a queued command.
            if (constraintOwner.m_constraintAddRemoveCounter.m_storage != 0) {
                if (constraintOwner.m_constraintAddRemoveCounter.m_storage > 0) {
                    // EXE-INLINED none: hkpAddConstraintToCriticalLockedIslandPhysicsCommand's constructor
                    hkpAddConstraintToCriticalLockedIslandPhysicsCommand command;
                    command.m_type = hkpPhysicsCommand::TYPE_ADD_CONSTRAINT_TO_LOCKED_ISLAND;
                    command.m_object0 = constraintOwner.m_constraintForCommand;
                    command.m_object1 = constraintOwner.m_callbackRequestForAddConstraint.m_storage;
                    constraintOwner.m_commandQueue.addCommand(command);
                    constraintOwner.m_callbackRequestForAddConstraint.m_storage = 0;
                }
                else {
                    // EXE-INLINED none: hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand's constructor
                    hkpRemoveConstraintFromCriticalLockedIslandPhysicsCommand command;
                    command.m_type = hkpPhysicsCommand::TYPE_REMOVE_CONSTRAINT_FROM_LOCKED_ISLAND;
                    command.m_object = constraintOwner.m_constraintForCommand;
                    constraintOwner.m_commandQueue.addCommand(command);
                }
            }
            else {
                constraintOwner.m_callbackRequestForAddConstraint.m_storage = 0;
            }
            constraintOwner.m_constraintAddRemoveCounter.m_storage = 0;
            if (hkOutOfMemoryState == hkMemoryAllocator::MEMORY_STATE_OUT_OF_MEMORY) {
                break;
            }
        }

        // An entry job always has its task block.
        hkpAgentSectorHeader::JobInfo* jobInfo = job.m_header->getJobInfos()[static_cast<int16_t>(job.m_taskIndex)];
        jobInfo->m_constraintInfo = constraintOwner.m_constraintInfo;
        const int32_t size = static_cast<int32_t>(constraintOwner.m_commandQueue.m_size);
        if (size) {
            // EXE-INLINED none: hkString::memCpy16 (the queues' buffers are untyped command bytes)
            uint32_t* dst32 = reinterpret_cast<uint32_t*>(jobInfo->m_commandQueue.m_start.val());
            const uint32_t* src32 = reinterpret_cast<const uint32_t*>(constraintOwner.m_commandQueue.m_start.val());
            for (int32_t numQuads = size >> 4; numQuads > 0; --numQuads) {
                dst32[0] = src32[0];
                dst32[1] = src32[1];
                dst32[2] = src32[2];
                dst32[3] = src32[3];
                dst32 += 4;
                src32 += 4;
            }
            jobInfo->m_commandQueue.m_size = static_cast<uint32_t>(size);
        }
        HK_TIMER_NAMED_END("NarrowPhaseTOI");
    }

    const hkJobQueue::JobStatus status =
        jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
    // EXE-INLINED none: hkpProcessCollisionInput::~hkpProcessCollisionInput
    if (input.m_spareAgentSector) {
        hkMemoryRouter::getInstance().m_heap->blockFree(input.m_spareAgentSector, sizeof(hkpAgent1nSector));
    }
    return status;
}

void hkpDeferredConstraintOwner::addConstraintToCriticalLockedIsland(hkpConstraintInstance* constraint) {
    ++m_constraintAddRemoveCounter.m_storage;
    m_constraintForCommand = constraint;
}

void hkpDeferredConstraintOwner::removeConstraintFromCriticalLockedIsland(hkpConstraintInstance* constraint) {
    --m_constraintAddRemoveCounter.m_storage;
    m_constraintForCommand = constraint;
}

void hkpDeferredConstraintOwner::addCallbackRequest(hkpConstraintInstance* constraint, int32_t request) {
    if (constraint->getInternal()) {
        constraint->getInternal()->m_callbackRequest |= request;
    }
    else {
        m_callbackRequestForAddConstraint.m_storage |= request;
    }
}
