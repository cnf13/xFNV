#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// What a ray cast against a shape reports to its collector: the surface normal at the hit, the fraction of the ray
// travelled and a shape-specific extra value.
struct hkpShapeRayCastCollectorOutput {
    hkpShapeRayCastCollectorOutput();
    hkpShapeRayCastCollectorOutput(const hkpShapeRayCastCollectorOutput* other);

    hkpShapeRayCastCollectorOutput& operator=(const hkpShapeRayCastCollectorOutput& arOther);
    // Whether the ray hit (m_hitFraction below one).
    hkBool hasHit() const;
    hkpShapeRayCastCollectorOutput* hkpShapeRayCastCollectorOutput_();

    hkVector4 m_normal;
    float m_hitFraction;
    int32_t m_extraInfo;
    int32_t m_pad[2];
};

inline hkpShapeRayCastCollectorOutput* hkpShapeRayCastCollectorOutput::hkpShapeRayCastCollectorOutput_() {
    this->~hkpShapeRayCastCollectorOutput();
    ::new (this) hkpShapeRayCastCollectorOutput;
    return this;
}

inline hkpShapeRayCastCollectorOutput&
hkpShapeRayCastCollectorOutput::operator=(const hkpShapeRayCastCollectorOutput& arOther) {
    m_normal = arOther.m_normal;
    m_hitFraction = arOther.m_hitFraction;
    m_extraInfo = arOther.m_extraInfo;
    for (uint32_t i = 0; i < 2; ++i) {
        m_pad[i] = arOther.m_pad[i];
    }
    return *this;
}

inline hkpShapeRayCastCollectorOutput::hkpShapeRayCastCollectorOutput() : m_hitFraction(1.0f), m_extraInfo(-1) {}

inline hkBool hkpShapeRayCastCollectorOutput::hasHit() const {
    hkBool kResult(m_hitFraction < 1.0);
    return kResult;
}
