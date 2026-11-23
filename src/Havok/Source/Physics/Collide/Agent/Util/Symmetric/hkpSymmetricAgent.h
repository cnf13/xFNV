#pragma once

#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/BvTreeStream/hkpBvTreeStreamAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/List/hkpListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/CompoundAgent/ShapeCollection/hkpShapeCollectionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/ConvexList/hkpConvexListAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Deprecated/MultiSphere/hkpMultiSphereAgent.h"
#include "Havok/Source/Physics/Collide/Agent/HeightFieldAgent/hkpHeightFieldAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Bv/hkpBvAgent.h"
#include "Havok/Source/Physics/Collide/Agent/MiscAgent/Transform/hkpTransformAgent.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Symmetric/hkpSymmetricAgentLinearCast.h"

// The agent for a flipped shape pair, adding the linear cast to hkpSymmetricAgentLinearCast: TAgent's linear cast with
// the bodies swapped, the points flipped back through an hkpSymmetricAgentFlipCastCollector. The layout is TAgent's.
template <class TAgent>
class hkpSymmetricAgent : public hkpSymmetricAgentLinearCast<TAgent> {
public:
    // The SDK's constructor: hkpSymmetricAgentLinearCast's.
    using hkpSymmetricAgentLinearCast<TAgent>::hkpSymmetricAgentLinearCast;

    explicit hkpSymmetricAgent(hkpContactMgr* mgr) : hkpSymmetricAgentLinearCast<TAgent>(mgr) {}

    void linearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& collector,
        hkpCdPointCollector* startCollector
    ) override;
};
// TAgent's linear cast (a direct call) with bodies A and B swapped, along the negated path, through flip cast
// collectors.
template <class TAgent>
void hkpSymmetricAgent<TAgent>::linearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& collector,
    hkpCdPointCollector* startCollector
) {
    hkpSymmetricAgentFlipCastCollector flip(input.m_path, collector);
    hkpLinearCastCollisionInput flippedInput = input;
    flippedInput.m_path.setNeg4(input.m_path);
    if (startCollector) {
        hkpSymmetricAgentFlipCastCollector startFlip(input.m_path, *startCollector);
        TAgent::linearCast(bodyB, bodyA, flippedInput, flip, &startFlip);
    }
    else {
        TAgent::linearCast(bodyB, bodyA, flippedInput, flip, nullptr);
    }
}
