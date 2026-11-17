#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpPairLinearCastCommand;
struct hkpPairLinearCastJob;
class hkpProcessCollisionInput;

// Havok SDK signature (hkpCollide.lib, tied by reloc).
void hkCpuPairLinearCastImplementation(
    const hkpPairLinearCastJob& pairLinearCastJob,
    const hkpProcessCollisionInput& collisionInput,
    hkpPairLinearCastCommand* commandsBase,
    int32_t numCommands
);
hkJobQueue::JobStatus hkCpuPairLinearCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
