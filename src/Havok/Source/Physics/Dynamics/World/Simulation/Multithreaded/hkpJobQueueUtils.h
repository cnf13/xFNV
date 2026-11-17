#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

struct hkpBuildJacobianTaskHeader;
class hkpDynamicsJob;

// Usercall (0xD96970, ret 0): when localTaskHeader (ecx) requests a split check (+0x21), adds
// a split-island job made from dynamicsJob (eax; its +0x10..+0x1C) to queue (the first stack argument, ecx of
// addJobQueueLocked) with data (the second, that call's first argument).
void spawnSplitSimulationIslandJob(const hkpBuildJacobianTaskHeader& localTaskHeader, hkJobQueue& queue,
	hkJobQueue::DynamicData* data, const hkpDynamicsJob* dynamicsJob);
// Havok SDK (the PDB's spawnBuildJacobiansJobs, hkpDynamicsJobQueueUtils.cpp): writes a
// build-jacobians job (type 4, size 0x30) into newJobCreated from dynamicsJob's island index, island, task header
// and thread structure (+0x10..+0x1C) and taskHeader (+0x4C, +0x58, +0x68), then sets the header's open jobs from
// +0x6C. Usercall (0xD96870): taskHeader in ecx, dynamicsJob in edx, newJobCreated in edi, ret 0.
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
