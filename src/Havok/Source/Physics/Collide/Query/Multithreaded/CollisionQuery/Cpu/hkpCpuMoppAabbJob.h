#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

hkJobQueue::JobStatus hkCpuMoppAabbQueryJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
