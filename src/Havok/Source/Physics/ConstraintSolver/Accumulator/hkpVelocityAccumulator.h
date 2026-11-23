#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// What the constraint solver keeps for one body while it solves: the velocities, the inverse masses and the scratch
// registers it works in. Not polymorphic.
class hkpVelocityAccumulator {
public:
    enum hkpAccumulatorType : int32_t {
        HK_RIGID_BODY = 0x0,
        HK_KEYFRAMED_RIGID_BODY = 0x1,
        HK_NO_GRAVITY_RIGID_BODY = 0x2,
        HK_END = 0x3,
    };

    // Sets this up as a copy of arSource for a mass-changer or
    // centre-of-mass-changer modifier.
    void copyVelocityAndScratch(const hkpVelocityAccumulator& arSource);

    void setFixed() {
        m_gravityFactor = 0.0f;
        m_type = HK_KEYFRAMED_RIGID_BODY;
        const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
        m_invMasses.m_quad = zero;
        m_linearVel.m_quad = zero;
        m_angularVel.m_quad = zero;
        m_scratch0.m_quad = zero;
        m_scratch123.m_col0.m_quad = zero;
        m_scratch123.m_col1.m_quad = zero;
        m_scratch123.m_col2.m_quad = zero;
    }
    hkpVelocityAccumulator& operator=(const hkpVelocityAccumulator& arSource);

    hkEnum<hkpAccumulatorType, uint8_t> m_type;
    uint32_t m_deactivationClass;
    float m_gravityFactor;
    hkVector4 m_linearVel;
    hkVector4 m_angularVel;
    hkVector4 m_invMasses;
    hkVector4 m_scratch0;
    hkRotation m_scratch123;
};
