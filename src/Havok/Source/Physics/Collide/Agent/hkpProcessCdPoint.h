#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A contact point an agent produced: the point and the id the contact manager gave it.
struct alignas(16) hkpProcessCdPoint {
    hkContactPoint m_contact;
    hkPadSpu<uint32_t> m_contactPointId;
};

int32_t contactPointIsValid(
    const hkpProcessCdPoint* point,
    float radius,
    const float* tolerance,
    float margin,
    const hkContactPoint* reference
);
