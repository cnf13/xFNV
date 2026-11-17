#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

struct hkpBuildJacobianTaskHeader;
struct hkpMtThreadStructure;

// Havok SDK (the PDB's deleteTaskHeaderResources, hkpCpuBroadPhaseJob.cpp, which has no header): frees the task
// header's task lists (+0x68, +0x78), its schema buffer (+0x14) and arrays, then the header.
// Usercall (0xD93AB0): taskHeader in esi, ret 0.
void deleteTaskHeaderResources(hkpBuildJacobianTaskHeader* taskHeader);

// The CPU jobs the multithreaded simulation runs without a job queue of its own: plain cdecl functions the queue
// dispatches through the job's function pointers. The PDB declares them as static members of this struct (its source
// file is hkpCpuFireJacobianSetupCallbackJob.cpp; cpuBroadPhaseJob's body is in hkpCpuBroadPhaseJob.cpp), both of
// type JobStatus (hkpMtThreadStructure&, hkJobQueue&, hkJobQueue::JobQueueEntry&). The PDB's parameter names
// (delPairs, newPairs, assertBuf / in, assertBuf, ostr) do not fit the exe's signatures; these are chosen here.
struct hkpSingleThreadedJobsOnIsland {
	// Broad phase update for one island: adds and removes the pairs, wakes the island's bodies and takes the next
	// job.
	static hkJobQueue::JobStatus cpuBroadPhaseJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
		hkJobQueue::JobQueueEntry& nextJobOut);
	// Fires the contact callbacks of the island's constraints and takes the next job.
	static hkJobQueue::JobStatus cpuFireJacobianSetupCallbackJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue,
		hkJobQueue::JobQueueEntry& jobInOut);
};
