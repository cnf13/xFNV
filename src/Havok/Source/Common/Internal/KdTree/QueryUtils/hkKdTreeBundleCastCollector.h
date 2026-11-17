#pragma once

#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

// The interface a k-d tree bundle cast (four rays at once) reports its mid phase hits to. It holds no state.
class hkKdTreeBundleCastCollector {
public:
    virtual ~hkKdTreeBundleCastCollector();

    // The primitive id is the SDK's hkPrimitiveId (pointer-sized): the overrides use it as the hkpCollidable*.
    virtual void addMidPhaseHit(hkPrimitiveId primitiveId, const hkVector4Comparison& mask) = 0;
    virtual void getHitFractions(hkVector4& fractionsOut) const = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
