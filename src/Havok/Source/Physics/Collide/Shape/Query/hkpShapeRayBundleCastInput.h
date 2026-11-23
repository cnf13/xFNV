#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkpRayShapeCollectionFilter;

// Four rays cast against a shape at once, stored transposed: m_from[i] and m_to[i] hold coordinate i (x, y, z) of the
// four rays' ends.
struct hkpShapeRayBundleCastInput {
    hkVector4 m_from[3];
    hkVector4 m_to[3];
    uint32_t m_filterInfo;
    const hkpRayShapeCollectionFilter* m_rayShapeCollectionFilter;
};
