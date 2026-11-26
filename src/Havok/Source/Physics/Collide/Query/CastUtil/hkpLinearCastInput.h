#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// The input of a linear cast: where the shape moves to and the tolerances of the cast.
class hkpLinearCastInput {
public:
    hkpLinearCastInput();

    hkVector4 m_to;
    float m_maxExtraPenetration;
    float m_startPointTolerance;
};

inline hkpLinearCastInput::hkpLinearCastInput() : m_maxExtraPenetration(0x1p-23f), m_startPointTolerance(0x1p-23f) {}
