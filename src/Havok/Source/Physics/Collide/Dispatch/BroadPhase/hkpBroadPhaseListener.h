#pragma once

#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"

// What the broad phase tells when a pair of handles begins or ends to overlap. The class has only a vtable; both
// callbacks are pure.
class hkpBroadPhaseListener {
public:
    virtual ~hkpBroadPhaseListener();

    virtual void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) = 0;
    virtual void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
