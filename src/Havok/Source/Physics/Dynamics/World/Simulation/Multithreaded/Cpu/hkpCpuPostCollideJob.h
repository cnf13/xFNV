#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

struct hkpMtThreadStructure;

// One island's post-collide work on a worker thread, then the next job (PDB hkCpuPostCollideJob,
// hkpCpuPostCollideJob.cpp; 0xD931A0, cdecl, ret 0). The exe tests the queue for null.
hkJobQueue::JobStatus hkCpuPostCollideJob(hkpMtThreadStructure& tl, hkJobQueue* jobQueue,
	hkJobQueue::JobQueueEntry& nextJobOut);
