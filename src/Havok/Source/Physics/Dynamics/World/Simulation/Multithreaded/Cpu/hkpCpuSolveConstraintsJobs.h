#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpMtThreadStructure;

hkJobQueue::JobStatus
hkCpuSolveApplyGravityJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
hkJobQueue::JobStatus
hkCpuSolveIntegrateVelocitiesJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
hkJobQueue::JobStatus
hkCpuSolveExportResultsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
hkJobQueue::JobStatus
hkCpuSolveConstraintsJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
// Solves one task's constraint batch and takes the next job.
hkJobQueue::JobStatus
hkCpuSolveConstraintBatchJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& jobInOut);
