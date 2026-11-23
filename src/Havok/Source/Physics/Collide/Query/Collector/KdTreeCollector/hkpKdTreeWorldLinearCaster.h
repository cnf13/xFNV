#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <xmmintrin.h>

class hkpCdPointCollector;
class hkpCollidable;
class hkpCollidableCollidableFilter;
class hkpCollisionAgentConfig;
class hkpLinearCastInput;

// Performs world linear casts using the KD-tree candidate collector.
class hkpKdTreeWorldLinearCaster : public hkKdTreeCastCollector {
public:
    hkBool addMidPhaseHit(hkPrimitiveId primitiveId) override;
    hkBool hasHit() override;
    float getHitFraction() override;

    void linearCast(
        const hkKdTree* tree,
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        const hkpCollidableCollidableFilter* filter,
        const hkpCollisionInput& collInput,
        hkpCollisionAgentConfig* config,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startPointCollector
    );

    const hkpLinearCastInput* m_input;
    const hkpCollidableCollidableFilter* m_filter;
    hkpCdPointCollector* m_castCollector;
    hkpCdPointCollector* m_startPointCollector;
    const hkpCollidable* m_collidableA;
    hkEnum<hkpShapeType, int32_t> m_typeA;
    // 4 bytes of padding: m_shapeInput is 16-byte aligned (its hkVector4 members), so it starts at 0x20.
    hkpLinearCastCollisionInput m_shapeInput;
};
