#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpMtThreadStructure;

// One island's post-collide work on a worker thread, then the next job. Tests the queue for null.
hkJobQueue::JobStatus
hkCpuPostCollideJob(hkpMtThreadStructure& tl, hkJobQueue* jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
