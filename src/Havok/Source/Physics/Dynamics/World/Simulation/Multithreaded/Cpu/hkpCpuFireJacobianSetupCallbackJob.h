#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpBuildJacobianTaskHeader;
struct hkpMtThreadStructure;

// Frees the task header's task lists, its schema buffer and arrays, then the header. Defined in
// hkpCpuBroadPhaseJob.cpp, which has no header.
void deleteTaskHeaderResources(hkpBuildJacobianTaskHeader* taskHeader);

// The CPU jobs the multithreaded simulation runs without a job queue of its own: plain functions the queue dispatches
// through the job's function pointers. cpuBroadPhaseJob's body is in hkpCpuBroadPhaseJob.cpp;
// cpuFireJacobianSetupCallbackJob's is in hkpCpuFireJacobianSetupCallbackJob.cpp.
struct hkpSingleThreadedJobsOnIsland {
	// Broad phase update for one island: adds and removes the pairs, wakes the island's bodies and takes the next
	// job.
	static hkJobQueue::JobStatus cpuBroadPhaseJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
		hkJobQueue::JobQueueEntry& nextJobOut);
	// Fires the contact callbacks of the island's constraints and takes the next job.
	static hkJobQueue::JobStatus cpuFireJacobianSetupCallbackJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
		hkJobQueue::JobQueueEntry& jobInOut);
};
