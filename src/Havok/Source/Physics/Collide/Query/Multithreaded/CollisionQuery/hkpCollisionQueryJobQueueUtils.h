#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

// The job queue callbacks of the collision query jobs.
class hkpCollisionQueryJobQueueUtils {
public:
	// The queue's finish callback for collision query jobs: closes the job in its shared header and, when it was
	// the batch's last open job, signals the waiting thread.
	static hkJobQueue::JobCreationStatus finishCollisionJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		const hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntryInput& newJobCreated);
	// The queue's pop callback for collision query jobs: copies the queue entry into the job the queue takes out and,
	// for the batched job types, hands out at most m_numCommandsPerTask commands, keeping the rest of the entry in the
	// queue.
	static hkJobQueue::JobPopFuncResult popCollisionJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut);
	// Registers the seven collision query job functions and the queue's pop and finish callbacks with queue for job
	// type 2.
	static void registerWithJobQueue(hkJobQueue* queue);
};
