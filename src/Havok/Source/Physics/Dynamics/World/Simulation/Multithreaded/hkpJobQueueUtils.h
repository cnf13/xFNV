#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpBuildJacobianTaskHeader;
class hkpDynamicsJob;

// When localTaskHeader requests a split check, adds a split-island job made from dynamicsJob to queue with data.
void spawnSplitSimulationIslandJob(const hkpBuildJacobianTaskHeader& localTaskHeader, hkJobQueue& queue,
	hkJobQueue::DynamicData* data, const hkpDynamicsJob* dynamicsJob);
// Writes a build-jacobians job (type 4) into newJobCreated from dynamicsJob's island index, island, task header and
// thread structure and from taskHeader, then sets the header's open jobs.
void spawnBuildJacobiansJobs(hkpBuildJacobianTaskHeader& taskHeader, const hkpDynamicsJob& dynamicsJob,
	hkJobQueue::JobQueueEntryInput& newJobCreated);

// Static dispatch helpers for the dynamics job queue; no instance state.
class hkpJobQueueUtils {
public:
	static hkJobQueue::JobPopFuncResult popIntegrateJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut);
	static hkJobQueue::JobPopFuncResult popCollideJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntry& jobOut);
	static hkJobQueue::JobCreationStatus finishCollideJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		const hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntryInput& newJobCreated);
	static hkJobQueue::JobCreationStatus finishIntegrateJob(hkJobQueue& queue, hkJobQueue::DynamicData* data,
		const hkJobQueue::JobQueueEntry& jobIn, hkJobQueue::JobQueueEntryInput& newJobCreated);
};
