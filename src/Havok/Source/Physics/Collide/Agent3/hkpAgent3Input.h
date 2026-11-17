#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"

class hkpCdBody;
class hkpContactMgr;
class hkpProcessCollisionInput;

// What an agent3 (stream collision agent) is given to create itself or to process: the two bodies, the collision input,
// the contact manager and the transform from body A's space to body B's. The hkPadSpu<T*> members are plain pointers on
// PC. m_aTb is 16-byte aligned.
class hkpAgent3Input {
public:
    const hkpCdBody* m_bodyA;
    const hkpCdBody* m_bodyB;
    const hkpCdBody* m_overrideBodyA;
    const hkpProcessCollisionInput* m_input;
    hkpContactMgr* m_contactMgr;
    hkTransform m_aTb;
};
