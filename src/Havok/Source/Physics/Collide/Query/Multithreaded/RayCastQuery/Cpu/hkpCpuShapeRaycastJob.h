#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

// Casts the ray of each command of a shape ray cast job against the command's collidables, into its results buffer
// or its collector, then finishes the job and takes the next one.
hkJobQueue::JobStatus hkCpuShapeRayCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
