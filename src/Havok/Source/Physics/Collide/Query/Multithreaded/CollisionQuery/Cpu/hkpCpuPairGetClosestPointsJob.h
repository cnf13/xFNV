#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Reconstruction.hpp"

struct hkpPairGetClosestPointsCommand;
struct hkpPairGetClosestPointsJob;
class hkpProcessCollisionInput;

// Havok SDK signature (hkpCollide.lib, tied by reloc).
void hkCpuPairGetClosestPointsImplementation(const hkpPairGetClosestPointsJob& pairGetClosestPointsJob,
	const hkpProcessCollisionInput& collisionInput, hkpPairGetClosestPointsCommand* commandsBase, int32_t numCommands);
hkJobQueue::JobStatus hkCpuPairGetClosestPointsJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
