#pragma once

#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

class hkpCdBody;

// A contact point found by collision detection, with the two bodies it is between. Declared as class to match the
// existing forward declarations.
class hkpCdPoint {
public:
    hkContactPoint m_contact;
    const hkpCdBody* m_cdBodyA;
    const hkpCdBody* m_cdBodyB;
};
