#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The collision tolerances of one collision quality: when contacts are kept or created and the continuous-physics
// separations.
class hkpCollisionQualityInfo {
public:
    float m_keepContact;
    float m_create4dContact;
    float m_createContact;
    float m_manifoldTimDistance;
    hkPadSpu<int32_t> m_useContinuousPhysics;
    hkBool m_useSimpleToiHandling;
    uint8_t padding15[3];
    float m_minSeparation;
    float m_minExtraSeparation;
    float m_minSafeDeltaTime;
    float m_minAbsoluteSafeDeltaTime;
    float m_toiSeparation;
    float m_toiExtraSeparation;
    float m_toiAccuracy;
    float m_maxContraintViolation;
    float m_minToiDeltaTime;
    uint16_t m_constraintPriority;
    hkBool m_enableToiWeldRejection;
    uint8_t padding3F;
};
