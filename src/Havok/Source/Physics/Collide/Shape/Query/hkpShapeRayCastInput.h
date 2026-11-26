#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkpRayShapeCollectionFilter;

// A ray cast against a shape: the ray's ends in the shape's space, the filter info of the ray and the filter that
// decides which children of a shape collection it may hit.
struct hkpShapeRayCastInput {
    hkpShapeRayCastInput();

    hkVector4 m_from;
    hkVector4 m_to;
    uint32_t m_filterInfo;
    const hkpRayShapeCollectionFilter* m_rayShapeCollectionFilter;
};

// The ray's ends are left as they are.
inline hkpShapeRayCastInput::hkpShapeRayCastInput() : m_filterInfo(0), m_rayShapeCollectionFilter(nullptr) {}
