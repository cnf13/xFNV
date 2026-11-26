#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A ray cast through the whole world: the ray's end points, whether a shape collection's filter is asked for each of
// its shapes, and the collision filter info the hit objects are filtered with.
struct hkpWorldRayCastInput {
    hkpWorldRayCastInput();

    hkVector4 m_from;
    hkVector4 m_to;
    hkBool m_enableShapeCollectionFilter;
    uint32_t m_filterInfo;
};

inline hkpWorldRayCastInput::hkpWorldRayCastInput() : m_enableShapeCollectionFilter(false), m_filterInfo(0) {}
