#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"

class hkContactPoint;
class hkpContactPointProperties;
class hkpDynamicsContactMgr;
class hkpVelocityAccumulator;

// A contact point of a collision as hkpContactListener::contactPointCallback sees it: the collision it belongs to,
// which kind of callback this is, the point and its properties, and the pointers a listener may write back (the
// separating velocity, the rotated normal, the shape keys).
class hkpContactPointEvent : public hkpCollisionEvent {
public:
    enum Type : int32_t {
        TYPE_TOI = 0,
        TYPE_EXPAND_MANIFOLD = 1,
        TYPE_MANIFOLD = 2,
        TYPE_MANIFOLD_AT_END_OF_STEP = 3,
        TYPE_MANIFOLD_FROM_SAVED_CONTACT_POINT = 4,
    };

    hkpContactPointEvent() = default;

    // The collision the event is about; the caller fills the rest.
    hkpContactPointEvent(
        CallbackSource aeSource,
        hkpRigidBody* apBodyA,
        hkpRigidBody* apBodyB,
        hkpDynamicsContactMgr* apMgr
    )
      : hkpCollisionEvent(aeSource, apBodyA, apBodyB, apMgr) {}

    hkpContactPointEvent(
        CallbackSource source,
        hkpRigidBody* bodyA,
        hkpRigidBody* bodyB,
        hkpDynamicsContactMgr* mgr,
        Type type,
        hkContactPoint* contactPoint,
        hkpContactPointProperties* contactPointProperties,
        float* separatingVelocity,
        float* rotateNormal,
        hkBool firingCallbacksForFullManifold,
        hkBool firstCallbackForFullManifold,
        hkBool lastCallbackForFullManifold,
        uint32_t* shapeKeyStorage,
        hkpVelocityAccumulator* accumulatorA,
        hkpVelocityAccumulator* accumulatorB
    )
      : hkpCollisionEvent(source, bodyA, bodyB, mgr) {
        m_type = type;
        m_contactPoint = contactPoint;
        m_contactPointProperties = contactPointProperties;
        m_firingCallbacksForFullManifold = firingCallbacksForFullManifold;
        m_firstCallbackForFullManifold = firstCallbackForFullManifold;
        m_lastCallbackForFullManifold = lastCallbackForFullManifold;
        m_separatingVelocity = separatingVelocity;
        m_rotateNormal = rotateNormal;
        m_shapeKeyStorage = shapeKeyStorage;
        m_accumulators[0] = accumulatorA;
        m_accumulators[1] = accumulatorB;
    }

    Type m_type;
    hkContactPoint* m_contactPoint;
    hkpContactPointProperties* m_contactPointProperties;
    hkBool m_firingCallbacksForFullManifold;
    hkBool m_firstCallbackForFullManifold;
    hkBool m_lastCallbackForFullManifold;
    float* m_separatingVelocity;
    float* m_rotateNormal;
    // Points to hkpShapeKey storage, a uint32_t per key.
    uint32_t* m_shapeKeyStorage;
    hkpVelocityAccumulator* m_accumulators[2];
};
