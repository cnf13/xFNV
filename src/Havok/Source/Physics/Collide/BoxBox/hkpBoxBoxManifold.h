#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxContactPoint.h"

class hkpCdBody;

// The contact points a box-box agent keeps between frames.
class hkpBoxBoxManifold {
public:
    static constexpr int32_t HK_BOXBOX_MANIFOLD_MAX_POINTS = 8;

    hkpBoxBoxManifold();

    void removePoint(int32_t i);
    int32_t addPoint(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpFeatureContactPoint& fcp);
    hkBool* findInManifold(hkBool* apResult, const hkpFeatureContactPoint& fcp);

    hkpFeatureContactPoint m_contactPoints[HK_BOXBOX_MANIFOLD_MAX_POINTS];
    uint8_t m_faceVertexFeatureCount;
    uint8_t m_numPoints;
    hkBool m_isComplete;
    hkBool m_manifoldNormalInitialized;
    uint32_t m_manifoldNormalB;
    hkVector4 m_manifoldNormalA;
};
