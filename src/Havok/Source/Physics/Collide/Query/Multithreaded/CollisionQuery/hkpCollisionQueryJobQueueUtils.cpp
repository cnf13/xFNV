#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobQueueUtils.h"

#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpMoppAabbJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpWorldLinearCastJob.h"

// IDA's sub_DF7A90; the PDB's name (hkpCollisionQueryJobQueueUtils::registerWithJobQueue, 0xDA3400, hands this
// address to the queue as the finish callback of job type 2). Sub-types 0-5 (one jump table target) decrement the
// shared header's open job count without a lock and release the job's semaphore at zero; every path returns
// NO_JOB_CREATED (1).
hkJobQueue::JobCreationStatus hkpCollisionQueryJobQueueUtils::finishCollisionJob(hkJobQueue& queue,
	hkJobQueue::DynamicData* data, const hkJobQueue::JobQueueEntry& jobIn,
	hkJobQueue::JobQueueEntryInput& newJobCreated) {
	// The queue entry is the job's raw bytes (src's JobQueueEntry is untyped); the exe reads them as the job.
	const hkpCollisionQueryJob& job = reinterpret_cast<const hkpCollisionQueryJob&>(jobIn);
	switch (job.m_jobSubType) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if (--job.m_sharedJobHeaderOnPpu->m_openJobs == 0) {
			job.atomicIncrementAndReleaseSemaphore();
		}
		break;
	default:
		break;
	}
	return hkJobQueue::NO_JOB_CREATED;
}

// 0xDF7AB0 is not a function entry: it is finishCollisionJob's shared NO_JOB_CREATED return (`mov eax, 1; ret`, the
// switch's `ja` target), which the registered body above returns inline, followed by the switch's jump table at
// 0xDF7AB8. IDA's def_DF7A9F spans it only because no function starts there.

// The queue's pop callback for collision query jobs, which the PDB places in
// .\Query\Multithreaded\CollisionQuery\hkpCollisionQueryJobQueueUtils.cpp: it copies the queue entry into the job the
// queue takes out and, for the five batched job types, hands out at most m_numCommandsPerTask commands, leaving the
// rest of the entry in the queue. The exe's register function stores 0xDF7AD0 as the pop callback; IDA has no
// function entry there, so the row's fragments at 0xDF7AB0 and 0xDF7B70 are pieces of it. The entry is
// `mov eax, [esp+0xC]`, the job in argument; the jump table of its switch is at 0xDF7BE0. cdecl, 4 stack arguments.
hkJobQueue::JobPopFuncResult hkpCollisionQueryJobQueueUtils::popCollisionJob(hkJobQueue& queue,
	hkJobQueue::DynamicData* data, hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut) {
	jobOut = jobIn;
	const hkpCollisionQueryJob& job = reinterpret_cast<const hkpCollisionQueryJob&>(jobIn);
	switch (static_cast<hkpCollisionQueryJob::JobSubType>(job.m_jobSubType)) {
	case hkpCollisionQueryJob::COLLISION_QUERY_PAIR_LINEAR_CAST: {
		hkpPairLinearCastJob& in = reinterpret_cast<hkpPairLinearCastJob&>(jobIn);
		hkpPairLinearCastJob& out = reinterpret_cast<hkpPairLinearCastJob&>(jobOut);
		if (in.m_numCommands <= in.m_numCommandsPerTask) {
			break;
		}
		out.m_numCommands = in.m_numCommandsPerTask;
		in.m_numCommands -= in.m_numCommandsPerTask;
		in.m_commandArray += in.m_numCommandsPerTask;
		return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
	}
	case hkpCollisionQueryJob::COLLISION_QUERY_WORLD_LINEAR_CAST: {
		hkpWorldLinearCastJob& in = reinterpret_cast<hkpWorldLinearCastJob&>(jobIn);
		hkpWorldLinearCastJob& out = reinterpret_cast<hkpWorldLinearCastJob&>(jobOut);
		if (in.m_numCommands <= in.m_numCommandsPerTask) {
			break;
		}
		out.m_numCommands = in.m_numCommandsPerTask;
		in.m_numCommands -= in.m_numCommandsPerTask;
		in.m_commandArray += in.m_numCommandsPerTask;
		return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
	}
	case hkpCollisionQueryJob::COLLISION_QUERY_MOPP_AABB: {
		hkpMoppAabbJob& in = reinterpret_cast<hkpMoppAabbJob&>(jobIn);
		hkpMoppAabbJob& out = reinterpret_cast<hkpMoppAabbJob&>(jobOut);
		if (in.m_numCommands <= in.m_numCommandsPerTask) {
			break;
		}
		out.m_numCommands = in.m_numCommandsPerTask;
		in.m_numCommands -= in.m_numCommandsPerTask;
		in.m_commandArray += in.m_numCommandsPerTask;
		return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
	}
	case hkpCollisionQueryJob::COLLISION_QUERY_PAIR_GET_CLOSEST_POINTS: {
		hkpPairGetClosestPointsJob& in = reinterpret_cast<hkpPairGetClosestPointsJob&>(jobIn);
		hkpPairGetClosestPointsJob& out = reinterpret_cast<hkpPairGetClosestPointsJob&>(jobOut);
		if (in.m_numCommands <= in.m_numCommandsPerTask) {
			break;
		}
		out.m_numCommands = in.m_numCommandsPerTask;
		in.m_numCommands -= in.m_numCommandsPerTask;
		in.m_commandArray += in.m_numCommandsPerTask;
		return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
	}
	case hkpCollisionQueryJob::COLLISION_QUERY_WORLD_GET_CLOSEST_POINTS: {
		hkpWorldGetClosestPointsJob& in = reinterpret_cast<hkpWorldGetClosestPointsJob&>(jobIn);
		hkpWorldGetClosestPointsJob& out = reinterpret_cast<hkpWorldGetClosestPointsJob&>(jobOut);
		if (in.m_numCommands <= in.m_numCommandsPerTask) {
			break;
		}
		out.m_numCommands = in.m_numCommandsPerTask;
		in.m_numCommands -= in.m_numCommandsPerTask;
		in.m_commandArray += in.m_numCommandsPerTask;
		return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
	}
	default:
		break;
	}
	return hkJobQueue::POP_QUEUE_ENTRY;
}

void hkpCollisionQueryJob::atomicIncrementAndReleaseSemaphore() const {
	int32_t* m_jobDoneFlag = reinterpret_cast<int32_t*>(this->m_jobDoneFlag);
	if (m_jobDoneFlag) {
		InterlockedExchangeAdd(m_jobDoneFlag, 1);
	}
	hkSemaphore* m_semaphore = this->m_semaphore;
	if (m_semaphore) {
		hkSemaphore::release(m_semaphore, 1);
	}
}
