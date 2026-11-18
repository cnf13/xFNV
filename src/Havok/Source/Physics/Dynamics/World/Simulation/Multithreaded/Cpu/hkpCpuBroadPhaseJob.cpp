#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuFireJacobianSetupCallbackJob.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "SDK/Windows/winbase.h"

// The SDK's deleteTaskHeaderResources: frees the header's build and solve task lists, its schema buffer (from the
// solver allocator), its entity array when the island was split, its new split islands' array, then the header.
void deleteTaskHeaderResources(hkpBuildJacobianTaskHeader* taskHeader) {
	while (taskHeader->m_tasks.m_buildJacobianTasks) {
		hkpBuildJacobianTask* task = taskHeader->m_tasks.m_buildJacobianTasks;
		taskHeader->m_tasks.m_buildJacobianTasks = task->m_next;
		// EXE-INLINED none: hkpBuildJacobianTask::operator delete
		hkMemoryRouter::getInstance().heap().blockFree(task, sizeof(hkpBuildJacobianTask));
	}
	while (taskHeader->m_solveTasks.m_firstSolveJacobiansTask) {
		hkpSolveConstraintBatchTask* task = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
		taskHeader->m_solveTasks.m_firstSolveJacobiansTask = task->m_next;
		// EXE-INLINED none: hkpSolveConstraintBatchTask::operator delete
		hkMemoryRouter::getInstance().heap().blockFree(task, sizeof(hkpSolveConstraintBatchTask));
	}
	if (taskHeader->m_buffer) {
		hkMemoryRouter::getInstance().solver().bufFree(taskHeader->m_buffer, taskHeader->m_bufferCapacity);
		taskHeader->m_buffer = nullptr;
	}
	if (taskHeader->m_numIslandsAfterSplit > 1) {
		// The SDK frees the array it allocated; the header holds it as const entities.
		hkMemoryRouter::getInstance().heap()._bufFree<hkpEntity*>(const_cast<hkpEntity**>(taskHeader->m_allEntities),
			taskHeader->m_entitiesCapacity);
		taskHeader->m_allEntities = nullptr;
	}
	// EXE-INLINED none: hkArray<hkpSimulationIsland*>::clearAndDeallocate
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	const int32_t capacityAndFlags = taskHeader->m_newSplitIslands.m_capacityAndFlags;
	taskHeader->m_newSplitIslands.m_size = 0;
	if (capacityAndFlags >= 0) {
		heap._bufFree<hkpSimulationIsland*>(taskHeader->m_newSplitIslands.m_data,
			capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::CAPACITY_MASK);
	}
	taskHeader->m_newSplitIslands.m_data = nullptr;
	taskHeader->m_newSplitIslands.m_capacityAndFlags = hkArrayBase<hkpSimulationIsland*>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkpBuildJacobianTaskHeader::operator delete
	hkMemoryRouter::getInstance().heap().blockFree(taskHeader, sizeof(hkpBuildJacobianTaskHeader));
}

// The broad phase of one island: marks the island inactive where the step asked for it, finds the island's new and
// lost pairs, reports the breached contact impulse limits once the export has finished (the last broad phase job of
// the task header), adds and removes the agents of the pairs, frees the task header when its last job is done and
// either queues the island's agent sector job or takes the next job.
hkJobQueue::JobStatus hkpSingleThreadedJobsOnIsland::cpuBroadPhaseJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& nextJobOut) {
	const hkpBroadPhaseJob& job = static_cast<const hkpBroadPhaseJob&>(static_cast<const hkJob&>(nextJobOut));
	hkpSimulationIsland* island = job.m_island;
	HK_TIMER_BEGIN("Broadphase", nullptr);

    if ((job.m_taskHeader->m_islandShouldBeDeactivated && tl.m_world->m_wantDeactivation.m_bool) &&
        (island->m_activeMark && !island->m_sparseEnabled)) {
        hkpWorldOperationUtil::markIslandInactiveMt(tl.m_world, island);
    }

    // Another thread sets the flag: it is read again on each test, and the pointer is tested for null first.
	volatile int8_t* exportFinished = &job.m_taskHeader->m_exportFinished;

	XFNV_INLINED_LOCAL(hkLocalArray<hkpBroadPhaseHandlePair>, newPairs, std::nullopt, std::nullopt);
	newPairs.m_data = nullptr;
	newPairs.m_size = 0;
	newPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
	{
		const int32_t iCapacity = tl.m_world->m_broadPhaseUpdateSize;
		// EXE-INLINED none: hkAllocateStack<hkpBroadPhaseHandlePair>. The stack allocator hands out untyped bytes.
		hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
		char* pBlock = stack.m_next.m_storage;
		stack.m_next.m_storage =
			pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpBroadPhaseHandlePair)) + 127) & ~127);
		newPairs.m_data = reinterpret_cast<hkpBroadPhaseHandlePair*>(pBlock);
		newPairs.m_capacityAndFlags =
			static_cast<uint32_t>(iCapacity) | hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
		newPairs.m_localMemory = reinterpret_cast<hkpBroadPhaseHandlePair*>(pBlock);
	}
	XFNV_INLINED_LOCAL(hkLocalArray<hkpBroadPhaseHandlePair>, delPairs, std::nullopt, std::nullopt);
	delPairs.m_data = nullptr;
	delPairs.m_size = 0;
	delPairs.m_capacityAndFlags = hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
	{
		const int32_t iCapacity = tl.m_world->m_broadPhaseUpdateSize;
		// EXE-INLINED none: hkAllocateStack<hkpBroadPhaseHandlePair>. The stack allocator hands out untyped bytes.
		hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
		char* pBlock = stack.m_next.m_storage;
		stack.m_next.m_storage =
			pBlock + ((iCapacity * static_cast<int32_t>(sizeof(hkpBroadPhaseHandlePair)) + 127) & ~127);
		delPairs.m_data = reinterpret_cast<hkpBroadPhaseHandlePair*>(pBlock);
		delPairs.m_localMemory = reinterpret_cast<hkpBroadPhaseHandlePair*>(pBlock);
		delPairs.m_capacityAndFlags =
			static_cast<uint32_t>(iCapacity) | hkArrayBase<hkpBroadPhaseHandlePair>::DONT_DEALLOCATE_FLAG;
	}

	tl.m_simulation.m_storage->collideEntitiesBroadPhaseContinuousFindPairs(island->m_entities.begin(),
		island->m_entities.getSize(), tl.m_world, newPairs, delPairs);

	// Each broad phase job of the header drops one reference; with one island no other thread shares the header.
	hkpBuildJacobianTaskHeader* taskHeader = job.m_taskHeader;
	const int32_t iNumIslands = taskHeader->m_numIslandsAfterSplit;
	const int32_t iReferencesWhenDone = 1 - iNumIslands;
	int32_t iReferencesLeft = 0;
	int32_t iOldReferences;
	if (iNumIslands == 1) {
		iOldReferences = taskHeader->m_referenceCount;
		taskHeader->m_referenceCount = iOldReferences - 1;
	} else {
		iOldReferences = InterlockedExchangeAdd(&taskHeader->m_referenceCount, -1);
	}

	if (iOldReferences == taskHeader->m_numIslandsAfterSplit) {
		if (exportFinished) {
			HK_TIMER_BEGIN("WaitForExport", nullptr);
			while (*exportFinished != 1 && *exportFinished != 2) {
			}
			HK_TIMER_END();
		}
		if (taskHeader->m_impulseLimitsBreached) {
			int32_t iNumBreached = 0;
			for (hkpImpulseLimitBreachedHeader* pHeader = taskHeader->m_impulseLimitsBreached; pHeader;
				pHeader = pHeader->m_next) {
				iNumBreached += pHeader->m_numBreached;
			}
			// EXE-INLINED none: hkAllocateStack<hkpContactImpulseLimitBreachedListenerInfo>. The stack allocator hands
			// out untyped bytes.
			hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
			char* pBlock = stack.m_next.m_storage;
			stack.m_next.m_storage = pBlock +
				((iNumBreached * static_cast<int32_t>(sizeof(hkpContactImpulseLimitBreachedListenerInfo)) + 127) &
					~127);
			hkpContactImpulseLimitBreachedListenerInfo* pInfos =
				reinterpret_cast<hkpContactImpulseLimitBreachedListenerInfo*>(pBlock);
			// Each header's breached elements follow it, one info's size each.
			int32_t iFirst = 0;
			for (hkpImpulseLimitBreachedHeader* pHeader = taskHeader->m_impulseLimitsBreached; pHeader;
				pHeader = pHeader->m_next) {
				hkString::memCpy4(&pInfos[iFirst], pHeader + 1,
					pHeader->m_numBreached *
						static_cast<int32_t>(sizeof(hkpContactImpulseLimitBreachedListenerInfo) / sizeof(uint32_t)));
				iFirst += pHeader->m_numBreached;
			}
			hkpWorldCallbackUtil::fireContactImpulseLimitBreached(tl.m_world, pInfos, iNumBreached);
			// EXE-INLINED none: hkDeallocateStack<hkpContactImpulseLimitBreachedListenerInfo>
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBlock;
		}
		*exportFinished = 2;
	} else {
		if (*exportFinished != 2) {
			HK_TIMER_BEGIN("WaitForSolverCallbacks", nullptr);
			while (*exportFinished != 2) {
			}
			HK_TIMER_END();
		}
		if (taskHeader->m_numIslandsAfterSplit > 2) {
			iReferencesLeft = InterlockedExchangeAdd(&taskHeader->m_referenceCount, -1) - 1;
		} else {
			iReferencesLeft = -1;
		}
	}

	if (newPairs.getSize() + delPairs.getSize() > 0) {
		HK_TIMER_BEGIN_LIST("AddRemoveAgnts", "Init");
		hkpWorld* world = tl.m_world;
		hkpEntity** entities = island->m_entities.begin();
		// EXE-INLINED none: hkpContinuousSimulation::removeAndAddPairs
		if (newPairs.getSize() + delPairs.getSize() > 0) {
			HK_TIMER_SPLIT_LIST("RemoveAgt");
			world->lockIslandForConstraintUpdate(entities[0]->getSimulationIsland());
			world->m_broadPhaseDispatcher->removePairs(static_cast<hkpTypedBroadPhaseHandlePair*>(delPairs.begin()),
				delPairs.getSize());
			if (hkMemoryRouter::getInstance().heap().canAllocTotal(newPairs.getSize() * 1024)) {
				HK_TIMER_SPLIT_LIST("AddAgt");
				world->m_broadPhaseDispatcher->addPairs(static_cast<hkpTypedBroadPhaseHandlePair*>(newPairs.begin()),
					newPairs.getSize(), world->m_collisionFilter);
			}
			world->unlockIslandForConstraintUpdate(entities[0]->getSimulationIsland());
		}
		HK_TIMER_END_LIST();
	}

	if (iReferencesLeft == iReferencesWhenDone) {
		// EXE-INLINED 0xD93AB0
		while (taskHeader->m_tasks.m_buildJacobianTasks) {
			hkpBuildJacobianTask* pTask = taskHeader->m_tasks.m_buildJacobianTasks;
			taskHeader->m_tasks.m_buildJacobianTasks = pTask->m_next;
			hkMemoryRouter::getInstance().heap().blockFree(pTask, static_cast<int32_t>(sizeof(hkpBuildJacobianTask)));
		}
		while (taskHeader->m_solveTasks.m_firstSolveJacobiansTask) {
			hkpSolveConstraintBatchTask* pTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
			taskHeader->m_solveTasks.m_firstSolveJacobiansTask = pTask->m_next;
			hkMemoryRouter::getInstance().heap().blockFree(pTask,
				static_cast<int32_t>(sizeof(hkpSolveConstraintBatchTask)));
		}
		if (taskHeader->m_buffer) {
			hkMemoryRouter::getInstance().solver().bufFree(taskHeader->m_buffer, taskHeader->m_bufferCapacity);
			taskHeader->m_buffer = nullptr;
		}
		if (taskHeader->m_numIslandsAfterSplit > 1) {
			hkMemoryRouter::getInstance().heap().bufFree(const_cast<hkpEntity**>(taskHeader->m_allEntities),
				taskHeader->m_entitiesCapacity * static_cast<int32_t>(sizeof(hkpEntity*)));
			taskHeader->m_allEntities = nullptr;
		}
		taskHeader->m_newSplitIslands.clearAndDeallocate();
		hkMemoryRouter::getInstance().heap().blockFree(taskHeader,
			static_cast<int32_t>(sizeof(hkpBuildJacobianTaskHeader)));
	}

	hkJobQueue::JobStatus status;
	const int32_t iNumSectors = island->m_agentTrack.m_sectors.getSize();
	if (iNumSectors > 0) {
		hkpWorld* world = tl.m_world;
		const int32_t iSectorsPerTask = world->m_maxSectorsPerCollideTask;
		// EXE-INLINED none: hkpAgentSectorJob::hkpAgentSectorJob
		hkpAgentSectorJob& sectorJob = static_cast<hkpAgentSectorJob&>(static_cast<hkJob&>(nextJobOut));
		sectorJob.m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_AGENT_SECTOR);
		sectorJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
		sectorJob.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
		sectorJob.m_size = static_cast<uint16_t>(sizeof(hkpAgentSectorJob));
		sectorJob.m_threadAffinity = -1;
		sectorJob.m_stepInfo = world->m_dynamicsStepInfo.m_stepInfo;
		sectorJob.m_taskIndex = 0;
		// The job holds the sector pointers as untyped elements, so they are cast to void* const*.
		sectorJob.m_elements = reinterpret_cast<void* const*>(island->m_agentTrack.m_sectors.begin());
		sectorJob.m_numElements = static_cast<uint16_t>(iNumSectors);
		sectorJob.m_numElementsPerTask = static_cast<uint16_t>(iSectorsPerTask);
		sectorJob.m_header = nullptr;
		sectorJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_COLLIDE);
		sectorJob.m_sectorIndex = 0;
		sectorJob.m_bytesUsedInLastSector = static_cast<uint16_t>(island->m_agentTrack.m_bytesUsedInLastSector);
		// m_numTotalTasks is written only when the sectors are split over several tasks.
		if (iNumSectors > iSectorsPerTask) {
			const int32_t iNumTasks = (iNumSectors - 1) / iSectorsPerTask + 1;
			sectorJob.m_header = hkpAgentSectorHeader::allocate(iNumTasks, 16);
			sectorJob.m_numTotalTasks = static_cast<uint16_t>(iNumTasks);
		}
		HK_TIMER_END();
		status = jobQueue.finishAddAndGetNextJob(HK_JOB_TYPE_DYNAMICS, hkJobQueue::JOB_LOW_PRIORITY, nextJobOut,
			hkJobQueue::WAIT_FOR_NEXT_JOB);
	} else {
		HK_TIMER_END();
		status = jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
	}

	delPairs.~hkLocalArray<hkpBroadPhaseHandlePair>();
	newPairs.~hkLocalArray<hkpBroadPhaseHandlePair>();
	return status;
}
