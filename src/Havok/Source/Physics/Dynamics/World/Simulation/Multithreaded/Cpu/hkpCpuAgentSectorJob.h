#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommandQueue.h"
#include "Reconstruction.hpp"

struct hkpMtThreadStructure;

// Collides an agent sector job's sectors or an agent entry job's entries, then finishes the job and takes the next
// one.
hkJobQueue::JobStatus
hkCpuAgentSectorJob(hkpMtThreadStructure& tl, hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);

// The constraint owner of an agent sector job: it does not change the world's constraints but queues the changes as
// commands, applied once the job is done.
class hkpDeferredConstraintOwner : public hkpConstraintOwner {
public:
    // Queues an add command for constraint.
    void addConstraintToCriticalLockedIsland(hkpConstraintInstance* constraint) override;
    // Queues a remove command for constraint.
    void removeConstraintFromCriticalLockedIsland(hkpConstraintInstance* constraint) override;
    void addCallbackRequest(hkpConstraintInstance* constraint, int32_t request) override;

    hkPadSpu<int32_t> m_constraintAddRemoveCounter;
    hkpConstraintInstance* m_constraintForCommand;
    hkPadSpu<int32_t> m_callbackRequestForAddConstraint;
    // The queue's 16-byte alignment puts it at 0x30.
    hkpFixedSizePhysicsCommandQueue<256> m_commandQueue;
};
inline constexpr fnvd::Vtable hkpDeferredConstraintOwner_vftable{};
