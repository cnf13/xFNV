#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

hkJobQueue::JobStatus hkCpuMoppAabbQueryJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
