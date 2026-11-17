#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

// Havok's character movement helper: one static function that turns a desired velocity into the character's new
// velocity on its surface.
class hkpCharacterMovementUtil {
public:
    // What calculateMovement works from: the gain that scales the velocity change and the surface frame the
    // character moves in. The first vector sits at 0x10 because hkVector4 is 16-byte aligned.
    struct hkpMovementUtilInput {
        float m_gain;
        hkVector4 m_forward;
        hkVector4 m_up;
        hkVector4 m_surfaceNormal;
        hkVector4 m_currentVelocity;
        hkVector4 m_desiredVelocity;
        float m_maxVelocityDelta;
        hkVector4 m_surfaceVelocity;
    };

    // Writes the velocity input asks for into velocityOut.
    static void calculateMovement(const hkpMovementUtilInput& input, hkVector4& velocityOut);
};
