#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

// The interface a k-d tree ray cast reports its mid phase hits to. It holds no state. addMidPhaseHit and hasHit take
// the hkBool result pointer as their first argument and return it.
class hkKdTreeCastCollector {
public:
    virtual ~hkKdTreeCastCollector();

    // The primitive id is the SDK's hkPrimitiveId (pointer-sized): the overrides use it as the hkpCollidable*.
    virtual hkBool addMidPhaseHit(hkPrimitiveId primitiveId) = 0;
    virtual hkBool hasHit() = 0;
    virtual float getHitFraction() = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
