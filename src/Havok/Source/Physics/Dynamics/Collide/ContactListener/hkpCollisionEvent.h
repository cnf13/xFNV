#pragma once

#include <cstdint>

class hkpDynamicsContactMgr;
class hkpRigidBody;

// A collision between two rigid bodies as the contact listeners see it: which body's listeners are being told, the two
// bodies, and the contact manager that holds the contact points.
class hkpCollisionEvent {
public:
    enum CallbackSource : int32_t {
        SOURCE_A = 0,
        SOURCE_B = 1,
        SOURCE_WORLD = 2,
    };

    // For callers that fill the fields themselves.
    hkpCollisionEvent() = default;

    hkpCollisionEvent(
        CallbackSource aeSource,
        hkpRigidBody* apBodyA,
        hkpRigidBody* apBodyB,
        hkpDynamicsContactMgr* apMgr
    )
      : m_source(aeSource), m_contactMgr(apMgr) {
        m_bodies[0] = apBodyA;
        m_bodies[1] = apBodyB;
    }

    hkpRigidBody* getBody(int32_t bodyIdx) const {
        return m_bodies[bodyIdx];
    }

    CallbackSource m_source;
    hkpRigidBody* m_bodies[2];
    hkpDynamicsContactMgr* m_contactMgr;
};
