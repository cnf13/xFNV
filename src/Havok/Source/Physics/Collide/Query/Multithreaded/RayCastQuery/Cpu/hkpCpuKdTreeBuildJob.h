#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

// Builds the k-d tree of the job's collidables in one task, then finishes the job and takes the next one.
hkJobQueue::JobStatus hkCpuKdTreeBuildJobFast(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
// Builds the k-d tree of the job's primitives, splitting it into four sub jobs from 8 primitives on.
hkJobQueue::JobStatus hkCpuKdTreeBuildFastDistributedJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
hkJobQueue::JobStatus hkCpuKdTreeBuildFastDistributedSubJob(hkJobQueue& jobQueue,
	hkJobQueue::JobQueueEntry& nextJobOut);
hkJobQueue::JobStatus hkCpuKdTreeBuildCoordinatorJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
hkJobQueue::JobStatus hkCpuKdTreeBuildSetupJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
