#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/Cpu/hkpCpuSolveConstraintsJobs.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Jacobian/hkpJacobianSchema.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolver.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverElemTemp.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverInfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

// Applies gravity to one range of an island's accumulators and takes the next job. The queue entry holds the job (an
// hkJob) in place.
hkJobQueue::JobStatus hkCpuSolveApplyGravityJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut) {
	HK_TIMER_BEGIN_LIST("Integrate", "AppGrav");
	hkpSolveApplyGravityJob& job = static_cast<hkpSolveApplyGravityJob&>(static_cast<hkJob&>(jobInOut));
	hkSolveApplyGravityByTheSteps(tl.m_world->m_dynamicsStepInfo.m_solverInfo, job.m_accumulators,
		job.m_accumulatorsEnd);
	HK_TIMER_END_LIST();
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Integrates the velocities of one range of an island's accumulators after a solver step, records in the job whether
// the solver has finished, and takes the next job. The queue entry holds the job (an hkJob) in place.
hkJobQueue::JobStatus hkCpuSolveIntegrateVelocitiesJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut) {
	HK_TIMER_BEGIN_LIST("Integrate", "IntVel");
	hkpSolveIntegrateVelocitiesJob& job = static_cast<hkpSolveIntegrateVelocitiesJob&>(static_cast<hkJob&>(jobInOut));
	hkBool outSolveFinished = false;
	hkSolveIntegrateVelocitiesByTheSteps(tl.m_world->m_dynamicsStepInfo.m_solverInfo,
		static_cast<int32_t>(job.m_currentSolverStep), job.m_accumulators, job.m_accumulatorsEnd, outSolveFinished);
	job.m_solvingFinished = outSolveFinished;
	HK_TIMER_END_LIST();
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Exports the solver results of one task's constraint batch for the multithreaded simulation and takes the next job.
// The queue entry holds the job from its first byte (the task pointer at +0x20).
hkJobQueue::JobStatus hkCpuSolveExportResultsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut) {
	HK_TIMER_BEGIN_LIST("Integrate", "SolverExport");
	// The queue entry and the job are both hkJobs; the SDK reads the entry as the job.
	const hkpSolveExportResultsJob& job =
		static_cast<const hkpSolveExportResultsJob&>(static_cast<const hkJob&>(jobInOut));
	const hkpSolveConstraintBatchTask* task = job.m_solveConstraintBatchTask;
	hkExportImpulsesAndRhs(tl.m_world->m_dynamicsStepInfo.m_solverInfo, task->m_solverElemTemp, task->m_schemas,
		task->m_accumulators);
	HK_TIMER_END_LIST();
	// Passes the job as both the finished job and the output entry, and waits for the next job.
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Solves an island's constraints in one job, queues the job that integrates the island's motions, exports the solver
// results and takes the next job. The queue entry holds the job (an hkJob) in place. The integrate-motion job is built
// in a stack queue entry of which only part is written, and addJob copies the whole entry into the queue, so some of
// the entry's bytes reach the queue unwritten, as the game does.
hkJobQueue::JobStatus hkCpuSolveConstraintsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut) {
	HK_TIMER_BEGIN_LIST("Integrate", "Solve");
	const hkpSolveConstraintsJob& job = static_cast<const hkpSolveConstraintsJob&>(static_cast<const hkJob&>(jobInOut));
	// The job's buffer is untyped memory carved up by byte offsets, so its three parts are cast from bytes.
	// EXE-INLINED none: hkAddByteOffset
	hkpVelocityAccumulator* accumulators =
		reinterpret_cast<hkpVelocityAccumulator*>(static_cast<char*>(job.m_buffer) + job.m_accumulatorsOffset);
	// EXE-INLINED none: hkAddByteOffset
	hkpJacobianSchema* schemas =
		reinterpret_cast<hkpJacobianSchema*>(static_cast<char*>(job.m_buffer) + job.m_schemasOffset);
	// EXE-INLINED none: hkAddByteOffset
	hkpSolverElemTemp* temp =
		reinterpret_cast<hkpSolverElemTemp*>(static_cast<char*>(job.m_buffer) + job.m_solverTempOffset);
	{
		// The solver temporaries are cleared as raw memory in 16-byte chunks, their size rounded up to 16 in 32 bits
		// and the loop skipped when the count is not positive.
		const int32_t numVectors = static_cast<int32_t>(((static_cast<uint32_t>(job.m_numSolverElemTemps) *
																 static_cast<uint32_t>(sizeof(hkpSolverElemTemp)) +
															 15u) &
															~15u) >>
			4);
		uint32_t* words = reinterpret_cast<uint32_t*>(temp);
		// EXE-INLINED none: hkString::memClear16
		for (int32_t i = 0; i < numVectors; ++i) {
			words[0] = 0;
			words[1] = 0;
			words[2] = 0;
			words[3] = 0;
			words += 4;
		}
	}
	hkSolveConstraints(tl.m_world->m_dynamicsStepInfo.m_solverInfo, schemas, accumulators, temp);
	{
		// The job that integrates the island's motions, built in a queue entry from this job and its task header.
		hkJobQueue::JobQueueEntry entry;
		hkpIntegrateMotionJob& integrateJob = static_cast<hkpIntegrateMotionJob&>(static_cast<hkJob&>(entry));
		hkpBuildJacobianTaskHeader* taskHeader = job.m_taskHeader;
		integrateJob.m_jobSubType = hkpDynamicsJob::DYNAMICS_JOB_INTEGRATE_MOTION;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		integrateJob.m_jobType.m_storage = HK_JOB_TYPE_DYNAMICS;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		integrateJob.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
		integrateJob.m_size = sizeof(hkpIntegrateMotionJob);
		integrateJob.m_threadAffinity = -1;
		integrateJob.m_islandIndex = job.m_islandIndex;
		integrateJob.m_island = job.m_island;
		integrateJob.m_taskHeader = taskHeader;
		integrateJob.m_mtThreadStructure = job.m_mtThreadStructure;
		integrateJob.m_solveConstraintBatchTask = taskHeader->m_solveTasks.m_firstSolveJacobiansTask;
		integrateJob.m_buffer = taskHeader->m_buffer;
		integrateJob.m_numEntities = taskHeader->m_numAllEntities;
		integrateJob.m_firstEntityIdx = 0;
		integrateJob.m_applyForcesAndStepMotionOnly = false;
		jobQueue.addJob(entry, hkJobQueue::JOB_HIGH_PRIORITY);
	}
	HK_TIMER_SPLIT_LIST("SolverExport");
	{
		const hkpBuildJacobianTaskHeader* taskHeader = job.m_taskHeader;
		hkExportImpulsesAndRhs(tl.m_world->m_dynamicsStepInfo.m_solverInfo, taskHeader->m_solverTempBase,
			taskHeader->m_schemasBase, taskHeader->m_accumulatorsBase);
	}
	HK_TIMER_END_LIST();
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

// Solves one task's constraint batch for the multithreaded simulation and takes the next job. The third argument is the
// queue entry, which holds the job from its first byte; the micro-step count comes from the world's solver info. The
// timer macros write into the stream's raw buffer, which is treated as untyped (hence the casts), as in hkpBvTreeAgent.
hkJobQueue::JobStatus hkCpuSolveConstraintBatchJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& jobInOut) {
	HK_TIMER_BEGIN_LIST("Integrate", "Solve");
	hkpSolveConstraintBatchJob* job = reinterpret_cast<hkpSolveConstraintBatchJob*>(&jobInOut);
	hkpSolveConstraintBatchTask* task = job->m_solveConstraintBatchTask;
	hkpVelocityAccumulator* accumulators = task->m_accumulators;
	hkpJacobianSchema* schemas = task->m_schemas;
	hkpSolverElemTemp* elemTemp = task->m_solverElemTemp;
	job->m_numSolverMicroSteps = tl.m_world->m_dynamicsStepInfo.m_solverInfo.m_numMicroSteps;
	if (job->m_currentSolverStep == 0) {
		// Zeroes the task's solver temporaries in 16-byte chunks, rounding the buffer's size up and skipping the loop
		// when the resulting count is not positive (a signed compare).
		const int32_t numVectors =
			static_cast<int32_t>((static_cast<uint32_t>(task->m_sizeOfSolverElemTempBuffer) + 15u) & ~15u) >> 4;
		uint32_t* words = reinterpret_cast<uint32_t*>(task->m_solverElemTemp);
		for (int32_t i = 0; i < numVectors; ++i) {
			words[0] = 0;
			words[1] = 0;
			words[2] = 0;
			words[3] = 0;
			words += 4;
		}
	}
	hkSolveConstraintsByTheSteps(tl.m_world->m_dynamicsStepInfo.m_solverInfo, job->m_currentSolverStep,
		job->m_currentSolverMicroStep, schemas, accumulators, elemTemp);
	HK_TIMER_END_LIST();
	// Passes the job as both the finished job and the output entry, and does not wait.
	return jobQueue.finishJobAndGetNextJob(&jobInOut, jobInOut, static_cast<hkJobQueue::WaitStatus>(0));
}
