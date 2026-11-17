#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpJobQueueUtils.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpMultithreadedSimulation.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"

// The SDK's spawnSplitSimulationIslandJob: queues, at high priority, the job that splits the job's island when the
// task header asks for a split check.
// KEEP: partcopy: the exe builds the 0x20-byte job in a 16-aligned stack local, writing bytes +0..+2, +4..+7,
// +0x10..+0x11 and +0x14..+0x1F only, and addJobQueueLocked (0xD04B00) enqueues a whole 128-byte JobQueueEntry copied
// from it: the unwritten bytes +3, +8..+0xF and +0x12..+0x13, and the 96 bytes past the local, reach the queue.
void spawnSplitSimulationIslandJob(const hkpBuildJacobianTaskHeader& localTaskHeader, hkJobQueue& queue,
	hkJobQueue::DynamicData* data, const hkpDynamicsJob* dynamicsJob) {
	if (localTaskHeader.m_splitCheckRequested) {
		alignas(16) hkpSplitSimulationIslandJob splitJob;
		// EXE-INLINED none: hkpSplitSimulationIslandJob::hkpSplitSimulationIslandJob
		splitJob.m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SPLIT_ISLAND);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		splitJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		splitJob.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
		splitJob.m_size = static_cast<uint16_t>(sizeof(hkpSplitSimulationIslandJob));
		splitJob.m_threadAffinity = -1;
		splitJob.m_islandIndex = dynamicsJob->m_islandIndex;
		splitJob.m_island = dynamicsJob->m_island;
		splitJob.m_taskHeader = dynamicsJob->m_taskHeader;
		splitJob.m_mtThreadStructure = dynamicsJob->m_mtThreadStructure;
		// The SDK passes the job as the queue entry it is copied into (the entry derives from hkJob).
		const hkJob& baseJob = splitJob;
		queue.addJobQueueLocked(data, static_cast<const hkJobQueue::JobQueueEntry&>(baseJob),
			hkJobQueue::JOB_HIGH_PRIORITY);
	}
}

// The SDK's spawnBuildJacobiansJobs: the island's build-jacobians job, one per build task, from dynamicsJob's island
// and taskHeader's tasks.
void spawnBuildJacobiansJobs(hkpBuildJacobianTaskHeader& taskHeader, const hkpDynamicsJob& dynamicsJob,
	hkJobQueue::JobQueueEntryInput& newJobCreated) {
	// The source's placement new of the job into the entry.
	hkJob* storage = &newJobCreated.m_job;
	if (storage != nullptr) {
		hkpBuildJacobiansJob* job = static_cast<hkpBuildJacobiansJob*>(storage);
		// EXE-INLINED none: hkpBuildJacobiansJob::hkpBuildJacobiansJob
		job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_BUILD_JACOBIANS);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
		job->m_size = static_cast<uint16_t>(sizeof(hkpBuildJacobiansJob));
		job->m_threadAffinity = -1;
		job->m_islandIndex = dynamicsJob.m_islandIndex;
		job->m_island = dynamicsJob.m_island;
		job->m_taskHeader = dynamicsJob.m_taskHeader;
		job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
		job->m_buildJacobianTask = taskHeader.m_tasks.m_buildJacobianTasks;
		job->m_constraintQueryIn = taskHeader.m_constraintQueryIn;
		job->m_finishSchemasWithGoto.m_bool = taskHeader.m_solveInSingleThread.m_bool;
	}
	taskHeader.m_openJobs = taskHeader.m_tasks.m_numBuildJacobianTasks;
	newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
	// EXE-INLINED none: hkEnum<E, S>::operator=
	newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
}

// The queue's pop callback for the integrate jobs: the job taken is a copy of the queue entry with its island looked
// up again in the world's active islands. A job covering more than one job's share (islands, entities, accumulators,
// tasks) hands out the first share and stays in the queue with the rest; the broad-phase job splits itself.
hkJobQueue::JobPopFuncResult hkpJobQueueUtils::popIntegrateJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
	hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut) {
	hkString::memCpy16NonEmpty(&jobOut, &jobIn, sizeof(hkJobQueue::JobQueueEntry) >> 4);
	hkpDynamicsJob& dynamicsJobIn = static_cast<hkpDynamicsJob&>(static_cast<hkJob&>(jobIn));
	hkpDynamicsJob& dynamicsJobOut = static_cast<hkpDynamicsJob&>(static_cast<hkJob&>(jobOut));
	// The exe reads the thread structure from the entry and the island index from the copy.
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
	case hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE:
		jobOut = jobIn;
		break;
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
		hkpVelocityAccumulator* split =
			in.m_accumulators + hkpSolveApplyGravityJob::MAX_NUM_ACCUMULATORS_FOR_APPLY_GRAVITY_JOB;
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
		hkpVelocityAccumulator* split =
			in.m_accumulators + hkpSolveIntegrateVelocitiesJob::MAX_NUM_ACCUMULATORS_FOR_INTEGRATE_VELOCITIES_JOB;
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
	default:
		break;
	}
	return hkJobQueue::POP_QUEUE_ENTRY;
}

// The queue's pop callback for the collide jobs: the job taken is a copy of the queue entry with its island looked up
// again in the world's active islands. An agent sector or agent entry job hands out m_numElementsPerTask elements and
// stays in the queue with the rest while it has more; a post-collide job is copied again and leaves the queue.
hkJobQueue::JobPopFuncResult hkpJobQueueUtils::popCollideJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
	hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut) {
	hkString::memCpy16NonEmpty(&jobOut, &jobIn, sizeof(hkJobQueue::JobQueueEntry) >> 4);
	// The queue entry is the job's raw bytes (src's JobQueueEntry is untyped); the exe reads them as the job.
	hkpDynamicsJob& dynamicsJobIn = reinterpret_cast<hkpDynamicsJob&>(jobIn);
	hkpDynamicsJob& dynamicsJobOut = reinterpret_cast<hkpDynamicsJob&>(jobOut);
	// The exe reads the thread structure from the entry and the island index from the copy.
	dynamicsJobOut.m_island =
		dynamicsJobIn.m_mtThreadStructure->m_world->m_activeSimulationIslands.m_data[dynamicsJobOut.m_islandIndex];
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
	case hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE:
		jobOut = jobIn;
		break;
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
	default:
		break;
	}
	return hkJobQueue::POP_QUEUE_ENTRY;
}

// The SDK's finish callback of the collide jobs: when the last agent sector job of an island finishes, it creates the
// island's post-collide job; no other collide job creates one.
hkJobQueue::JobCreationStatus hkpJobQueueUtils::finishCollideJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
	const hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntryInput& newJobCreated) {
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
	// The source's placement new of the post-collide job into the entry.
	hkJob* storage = &newJobCreated.m_job;
	if (storage != nullptr) {
		hkpPostCollideJob* postCollideJob = static_cast<hkpPostCollideJob*>(storage);
		// EXE-INLINED none: hkpPostCollideJob::hkpPostCollideJob
		postCollideJob->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		postCollideJob->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
		// EXE-INLINED none: hkEnum<E, S>::operator=
		postCollideJob->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
		postCollideJob->m_size = static_cast<uint16_t>(sizeof(hkpPostCollideJob));
		postCollideJob->m_threadAffinity = -1;
		// The exe stores the island index twice (the base's copy, then the job's own), the same value both times.
		postCollideJob->m_islandIndex = job.m_islandIndex;
		postCollideJob->m_island = job.m_island;
		postCollideJob->m_taskHeader = job.m_taskHeader;
		postCollideJob->m_mtThreadStructure = job.m_mtThreadStructure;
		postCollideJob->m_header = job.m_header;
	}
	newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
	return hkJobQueue::JOB_CREATED;
}

// The queue's finish callback for the integrate jobs: when the last open job of an island's stage finishes, it
// creates the job of the next stage (callbacks, Jacobians, gravity, constraint batches over the solver's micro steps
// and steps, velocities, motion, export, broad phase) and resets the task header's open-job count for it.
// KEEP (partcopy:): after a build-jacobians stage with m_splitCheckRequested set, the exe queues a split-island job
// built on its stack (0xD96B88): 0x20 bytes, of which bytes 0x3, 0x8..0xF and 0x12..0x13 are never written, and
// addJobQueueLocked's enqueue (0xD057A0) copies 0x80 bytes from it (`rep movsd`, 0x20 dwords) into the queue: those
// unwritten bytes and the 0x60 bytes of the exe's frame after the job.
hkJobQueue::JobCreationStatus hkpJobQueueUtils::finishIntegrateJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
	const hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntryInput& newJobCreated) {
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
		if (storage != nullptr) {
			hkpFireJacobianSetupCallback* job = static_cast<hkpFireJacobianSetupCallback*>(storage);
			// EXE-INLINED none: hkpFireJacobianSetupCallback::hkpFireJacobianSetupCallback
			job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_FIRE_JACOBIAN_SETUP_CALLBACK);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
			job->m_size = static_cast<uint16_t>(sizeof(hkpFireJacobianSetupCallback));
			job->m_threadAffinity = -1;
			job->m_islandIndex = dynamicsJob.m_islandIndex;
			job->m_island = dynamicsJob.m_island;
			job->m_taskHeader = dynamicsJob.m_taskHeader;
			job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
		}
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
			if (storage != nullptr) {
				hkpSolveConstraintsJob* job = static_cast<hkpSolveConstraintsJob*>(storage);
				// EXE-INLINED none: hkpSolveConstraintsJob::hkpSolveConstraintsJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINTS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveConstraintsJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = dynamicsJob.m_islandIndex;
				job->m_island = dynamicsJob.m_island;
				job->m_taskHeader = dynamicsJob.m_taskHeader;
				job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
				job->m_buffer = taskHeader->m_buffer;
				// The offsets of the accumulators, schemas and solver temporaries in the buffer, in bytes.
				job->m_accumulatorsOffset =
					static_cast<uint32_t>(static_cast<char*>(static_cast<void*>(taskHeader->m_accumulatorsBase)) -
						static_cast<char*>(job->m_buffer));
				job->m_schemasOffset =
					static_cast<uint32_t>(static_cast<char*>(static_cast<void*>(taskHeader->m_schemasBase)) -
						static_cast<char*>(job->m_buffer));
				job->m_solverTempOffset =
					static_cast<uint32_t>(static_cast<char*>(static_cast<void*>(taskHeader->m_solverTempBase)) -
						static_cast<char*>(job->m_buffer));
				job->m_bufferSize = static_cast<uint32_t>(taskHeader->m_bufferSize);
				job->m_numSolverResults = taskHeader->m_numSolverResults;
				job->m_numSolverElemTemps = taskHeader->m_numSolverElemTemps;
				createdJob = job;
			}
			newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
			// EXE-INLINED none: hkEnum<E, S>::operator=
			newJobCreated.m_job.m_jobSpuType.m_storage =
				static_cast<uint8_t>(taskHeader->m_solveInSingleThreadOnPpuOnly.m_bool != 0 ? HK_JOB_SPU_TYPE_DISABLED
																							: HK_JOB_SPU_TYPE_ENABLED);
		} else {
			if (storage != nullptr) {
				hkpSolveApplyGravityJob* job = static_cast<hkpSolveApplyGravityJob*>(storage);
				// EXE-INLINED none: hkpSolveApplyGravityJob::hkpSolveApplyGravityJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_APPLY_GRAVITY);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveApplyGravityJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = dynamicsJob.m_islandIndex;
				job->m_island = dynamicsJob.m_island;
				job->m_taskHeader = dynamicsJob.m_taskHeader;
				job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
				job->m_accumulators = taskHeader->m_accumulatorsBase;
				job->m_accumulatorsEnd = taskHeader->m_accumulatorsEnd;
				createdJob = job;
			}
			newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
			// EXE-INLINED none: hkEnum<E, S>::operator=
			newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
			taskHeader->m_openJobs = taskHeader->m_numApplyGravityJobs;
		}
		if (taskHeader->m_splitCheckRequested != 0) {
			// EXE-INLINED 0xD96970
			hkpSplitSimulationIslandJob splitJob;
			// EXE-INLINED none: hkpSplitSimulationIslandJob::hkpSplitSimulationIslandJob
			splitJob.m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SPLIT_ISLAND);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			splitJob.m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			splitJob.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
			splitJob.m_size = static_cast<uint16_t>(sizeof(hkpSplitSimulationIslandJob));
			splitJob.m_threadAffinity = -1;
			splitJob.m_islandIndex = createdJob->m_islandIndex;
			splitJob.m_island = createdJob->m_island;
			splitJob.m_taskHeader = createdJob->m_taskHeader;
			splitJob.m_mtThreadStructure = createdJob->m_mtThreadStructure;
			// The source's cast: the queue takes the job as an entry and copies a whole entry from it.
			queue.addJobQueueLocked(data,
				static_cast<const hkJobQueue::JobQueueEntry&>(static_cast<const hkJob&>(splitJob)),
				hkJobQueue::JOB_HIGH_PRIORITY);
		}
		return hkJobQueue::JOB_CREATED;
	}
	case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINTS:
		taskHeader->m_exportFinished = 1;
		return hkJobQueue::NO_JOB_CREATED;
	case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_APPLY_GRAVITY: {
		if (--taskHeader->m_openJobs != 0) {
			return hkJobQueue::NO_JOB_CREATED;
		}
		if (storage != nullptr) {
			hkpSolveConstraintBatchJob* job = static_cast<hkpSolveConstraintBatchJob*>(storage);
			// EXE-INLINED none: hkpSolveConstraintBatchJob::hkpSolveConstraintBatchJob
			job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
			job->m_size = static_cast<uint16_t>(sizeof(hkpSolveConstraintBatchJob));
			job->m_threadAffinity = -1;
			job->m_islandIndex = dynamicsJob.m_islandIndex;
			job->m_island = dynamicsJob.m_island;
			job->m_taskHeader = dynamicsJob.m_taskHeader;
			job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
			job->m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
			job->m_currentSolverStep = 0;
			job->m_currentSolverMicroStep = 0;
		}
		newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
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
			if (storage != nullptr) {
				hkpSolveConstraintBatchJob* job = static_cast<hkpSolveConstraintBatchJob*>(storage);
				// EXE-INLINED none: hkpSolveConstraintBatchJob::hkpSolveConstraintBatchJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveConstraintBatchJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = batchJob.m_islandIndex;
				job->m_island = batchJob.m_island;
				job->m_taskHeader = batchJob.m_taskHeader;
				job->m_mtThreadStructure = batchJob.m_mtThreadStructure;
				*job = batchJob;
				job->m_solveConstraintBatchTask = batchJob.m_solveConstraintBatchTask->m_firstTaskInNextBatch;
			}
			numOpenJobs = sizeOfNextBatch;
		} else if (batchJob.m_currentSolverMicroStep + 1 < batchJob.m_numSolverMicroSteps) {
			// The first batch of the next micro step.
			if (storage != nullptr) {
				hkpSolveConstraintBatchJob* job = static_cast<hkpSolveConstraintBatchJob*>(storage);
				// EXE-INLINED none: hkpSolveConstraintBatchJob::hkpSolveConstraintBatchJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveConstraintBatchJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = batchJob.m_islandIndex;
				job->m_island = batchJob.m_island;
				job->m_taskHeader = batchJob.m_taskHeader;
				job->m_mtThreadStructure = batchJob.m_mtThreadStructure;
				*job = batchJob;
				job->m_solveConstraintBatchTask = batchJob.m_solveConstraintBatchTask->m_firstTaskInNextBatch;
				++job->m_currentSolverMicroStep;
				job->m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
			}
			numOpenJobs = taskHeader->m_solveTasks.m_firstBatchSize;
		} else {
			// The step's micro steps are done: integrate the velocities.
			if (storage != nullptr) {
				hkpSolveIntegrateVelocitiesJob* job = static_cast<hkpSolveIntegrateVelocitiesJob*>(storage);
				// EXE-INLINED none: hkpSolveIntegrateVelocitiesJob::hkpSolveIntegrateVelocitiesJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveIntegrateVelocitiesJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = batchJob.m_islandIndex;
				job->m_island = batchJob.m_island;
				job->m_taskHeader = batchJob.m_taskHeader;
				job->m_mtThreadStructure = batchJob.m_mtThreadStructure;
				job->m_accumulators = taskHeader->m_accumulatorsBase;
				job->m_accumulatorsEnd = taskHeader->m_accumulatorsEnd;
				job->m_currentSolverStep = batchJob.m_currentSolverStep;
				job->m_solvingFinished.m_bool = 0;
			}
			numOpenJobs = taskHeader->m_numIntegrateVelocitiesJobs;
		}
		newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
		taskHeader->m_openJobs = numOpenJobs;
		return hkJobQueue::JOB_CREATED;
	}
	case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_INTEGRATE_VELOCITIES: {
		if (--taskHeader->m_openJobs != 0) {
			return hkJobQueue::NO_JOB_CREATED;
		}
		const hkpSolveIntegrateVelocitiesJob& velocitiesJob =
			static_cast<const hkpSolveIntegrateVelocitiesJob&>(dynamicsJob);
		if (velocitiesJob.m_solvingFinished.m_bool == 0) {
			// The next solver step, from the first batch.
			if (storage != nullptr) {
				hkpSolveConstraintBatchJob* job = static_cast<hkpSolveConstraintBatchJob*>(storage);
				// EXE-INLINED none: hkpSolveConstraintBatchJob::hkpSolveConstraintBatchJob
				job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_SOLVE_CONSTRAINT_BATCH);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
				// EXE-INLINED none: hkEnum<E, S>::operator=
				job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
				job->m_size = static_cast<uint16_t>(sizeof(hkpSolveConstraintBatchJob));
				job->m_threadAffinity = -1;
				job->m_islandIndex = velocitiesJob.m_islandIndex;
				job->m_island = velocitiesJob.m_island;
				job->m_taskHeader = velocitiesJob.m_taskHeader;
				job->m_mtThreadStructure = velocitiesJob.m_mtThreadStructure;
				job->m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
				job->m_currentSolverStep = velocitiesJob.m_currentSolverStep + 1;
				job->m_currentSolverMicroStep = 0;
			}
			newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
			// EXE-INLINED none: hkEnum<E, S>::operator=
			newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
			taskHeader->m_openJobs = taskHeader->m_solveTasks.m_firstBatchSize;
			return hkJobQueue::JOB_CREATED;
		}
		// Solving is finished: integrate the motions.
		if (storage != nullptr) {
			hkpIntegrateMotionJob* job = static_cast<hkpIntegrateMotionJob*>(storage);
			// EXE-INLINED none: hkpIntegrateMotionJob::hkpIntegrateMotionJob
			job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
			job->m_size = static_cast<uint16_t>(sizeof(hkpIntegrateMotionJob));
			job->m_threadAffinity = -1;
			job->m_islandIndex = velocitiesJob.m_islandIndex;
			job->m_island = velocitiesJob.m_island;
			job->m_taskHeader = velocitiesJob.m_taskHeader;
			job->m_mtThreadStructure = velocitiesJob.m_mtThreadStructure;
			job->m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
			job->m_firstEntityIdx = 0;
			job->m_numEntities = taskHeader->m_numAllEntities;
			job->m_buffer = taskHeader->m_buffer;
			job->m_applyForcesAndStepMotionOnly.m_bool = 0;
		}
		newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		newJobCreated.m_job.m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_ENABLED);
		taskHeader->m_openJobs = taskHeader->m_tasks.m_numBuildJacobianTasks;
		return hkJobQueue::JOB_CREATED;
	}
	case hkpDynamicsJob::DYNAMICS_JOB_SOLVE_EXPORT_RESULTS: {
		taskHeader->m_openJobs--;
		const hkpSolveExportResultsJob& exportJob = static_cast<const hkpSolveExportResultsJob&>(dynamicsJob);
		// The task's schema buffer is raw memory that starts with its block of breached impulse limits; a block with
		// any is linked into the task header's list.
		hkpImpulseLimitBreachedHeader* breached =
			reinterpret_cast<hkpImpulseLimitBreachedHeader*>(exportJob.m_solveConstraintBatchTask->m_schemas);
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
			numFinishedJobs = static_cast<int32_t>(static_cast<uint32_t>(motionJob.m_numEntities - 1) /
					hkpIntegrateMotionJob::ACCUMULATORS_PER_JOB +
				1);
		}
		dynamicsJob.m_taskHeader->m_numUnfinishedJobsForBroadphase -= numFinishedJobs;
		if (dynamicsJob.m_taskHeader->m_numUnfinishedJobsForBroadphase != 0) {
			return hkJobQueue::NO_JOB_CREATED;
		}
		if (storage != nullptr) {
			hkpBroadPhaseJob* job = static_cast<hkpBroadPhaseJob*>(storage);
			// EXE-INLINED none: hkpBroadPhaseJob::hkpBroadPhaseJob
			job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_BROADPHASE);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
			job->m_size = static_cast<uint16_t>(sizeof(hkpBroadPhaseJob));
			job->m_threadAffinity = -1;
			job->m_islandIndex = dynamicsJob.m_islandIndex;
			job->m_island = dynamicsJob.m_island;
			job->m_taskHeader = dynamicsJob.m_taskHeader;
			job->m_mtThreadStructure = dynamicsJob.m_mtThreadStructure;
			job->m_numIslands = 1;
		}
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
		if (storage != nullptr) {
			hkpPostCollideJob* job = static_cast<hkpPostCollideJob*>(storage);
			// EXE-INLINED none: hkpPostCollideJob::hkpPostCollideJob
			job->m_jobSubType = static_cast<uint8_t>(hkpDynamicsJob::DYNAMICS_JOB_POST_COLLIDE);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobType.m_storage = static_cast<uint8_t>(HK_JOB_TYPE_DYNAMICS);
			// EXE-INLINED none: hkEnum<E, S>::operator=
			job->m_jobSpuType.m_storage = static_cast<uint8_t>(HK_JOB_SPU_TYPE_DISABLED);
			job->m_size = static_cast<uint16_t>(sizeof(hkpPostCollideJob));
			job->m_threadAffinity = -1;
			// The exe stores the island index twice (the base's copy, then the job's own), the same value both times.
			job->m_islandIndex = sectorJob.m_islandIndex;
			job->m_island = sectorJob.m_island;
			job->m_taskHeader = sectorJob.m_taskHeader;
			job->m_mtThreadStructure = sectorJob.m_mtThreadStructure;
			job->m_header = sectorJob.m_header;
		}
		newJobCreated.m_jobPriority.m_storage = hkJobQueue::JOB_HIGH_PRIORITY;
		return hkJobQueue::JOB_CREATED;
	}
	default:
		return hkJobQueue::NO_JOB_CREATED;
	}
}
