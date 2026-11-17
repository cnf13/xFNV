#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// What a ray cast against a shape reports to its collector: the surface normal at the hit, the fraction of the ray
// travelled and a shape-specific extra value.
struct hkpShapeRayCastCollectorOutput {
    hkpShapeRayCastCollectorOutput();
    hkpShapeRayCastCollectorOutput(const hkpShapeRayCastCollectorOutput* a1);

    hkpShapeRayCastCollectorOutput& operator=(const hkpShapeRayCastCollectorOutput& arOther);
    // Whether the ray hit (m_hitFraction below one).
    hkBool hasHit() const;
    hkpShapeRayCastCollectorOutput* hkpShapeRayCastCollectorOutput_();

    hkVector4 m_normal;
    float m_hitFraction;
    int32_t m_extraInfo;
    int32_t m_pad[2];
};
