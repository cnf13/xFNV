#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"

#include <stdint.h>

class hkpCdBody;
class hkpCollidable;
class hkpConstraintOwner;
class hkpDynamicsContactMgr;
class hkpEntity;
class hkpGskCache;
class hkpProcessCollisionData;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;
class hkpSimpleContactConstraintData;

// The older contact events an hkpContactListener receives through contactPointAddedCallback,
// contactPointRemovedCallback and contactProcessCallback. The listener class hkpCollisionListener itself is not
// declared.

// Whether a new contact point is kept.
enum hkpContactPointAccept : int32_t {
    HK_CONTACT_POINT_ACCEPT = 0,
    HK_CONTACT_POINT_REJECT = 1,
};

// A contact point was added between two bodies.
struct hkpContactPointAddedEvent {
    enum Type : int32_t {
        TYPE_TOI = 0,
        TYPE_MANIFOLD = 1,
    };

    hkpContactPointAddedEvent() = default;
    // The constructor leaves the firing entity as it is.
    hkpContactPointAddedEvent(
        hkpDynamicsContactMgr* contactMgr,
        const hkpProcessCollisionInput* collisionInput,
        hkpProcessCollisionOutput* collisionOutput,
        const hkpCdBody* a,
        const hkpCdBody* b,
        hkContactPoint* cp,
        const hkpGskCache* gskCache,
        hkpContactPointProperties* cpp,
        float projectedVelocity,
        Type pointType
    );

    const hkpCdBody* m_bodyA;
    const hkpCdBody* m_bodyB;
    hkEnum<Type, int32_t> m_type;
    // The entity whose listeners are being told.
    hkpEntity* m_callbackFiredFrom;
    const hkContactPoint* m_contactPoint;
    const hkpGskCache* m_gskCache;
    hkpContactPointProperties* m_contactPointProperties;
    float m_projectedVelocity;
    hkpContactPointAccept m_status;
    hkpDynamicsContactMgr* m_internalContactMgr;
    const hkpProcessCollisionInput* m_collisionInput;
    hkpProcessCollisionOutput* m_collisionOutput;
};

// A contact point was added at a time of impact: the added event with that time (the SDK's hkTime).
struct hkpToiPointAddedEvent : public hkpContactPointAddedEvent {
    float m_toi;
};

// A contact point was added to a collision's manifold: the added event with the point's id and how many steps pass
// before its next process callback.
struct hkpManifoldPointAddedEvent : public hkpContactPointAddedEvent {
    hkpManifoldPointAddedEvent(
        uint16_t a1,
        hkpDynamicsContactMgr* a2,
        const hkpProcessCollisionInput* a3,
        hkpProcessCollisionOutput* a4,
        const hkpCdBody* a5,
        const hkpCdBody* a6,
        hkContactPoint* a7,
        const hkpGskCache* a8,
        hkpContactPointProperties* a9,
        float a10
    );

    uint16_t m_contactPointId;
    uint16_t m_nextProcessCallbackDelay;
};

// A contact point was removed.
struct hkpContactPointRemovedEvent {
    uint16_t m_contactPointId;
    hkpContactPointProperties* m_contactPointProperties;
    hkpEntity* m_entityA;
    hkpEntity* m_entityB;
    // The entity whose listeners are being told.
    hkpEntity* m_callbackFiredFrom;
    hkpDynamicsContactMgr* m_internalContactMgr;
    hkpConstraintOwner* m_constraintOwner;
};

// A contact point was confirmed: the solver will use it.
struct hkpContactPointConfirmedEvent {
    const hkpCollidable* m_collidableA;
    const hkpCollidable* m_collidableB;
    // The entity whose listeners are being told.
    hkpEntity* m_callbackFiredFrom;
    hkContactPoint* m_contactPoint;
    hkpContactPointProperties* m_contactPointProperties;
    float m_rotateNormal;
    float m_projectedVelocity;
    hkEnum<hkpContactPointAddedEvent::Type, int32_t> m_type;
    const hkpSimpleContactConstraintData* m_contactData;
};

// The contact points of a collision were processed this step.
struct hkpContactProcessEvent {
    const hkpCollidable* m_collidableA;
    const hkpCollidable* m_collidableB;
    // The entity whose listeners are being told.
    hkpEntity* m_callbackFiredFrom;
    hkpProcessCollisionData* m_collisionData;
    hkpContactPointProperties* m_contactPointProperties[256];
    hkpDynamicsContactMgr* m_internalContactMgr;
};

// The older listener interface: a contact listener that also gets the confirmed event.
class hkpCollisionListener : public hkpContactListener {
public:
    void contactPointAddedCallback(hkpContactPointAddedEvent& event) override;
    void contactPointRemovedCallback(hkpContactPointRemovedEvent& event) override;
    void contactProcessCallback(hkpContactProcessEvent& event) override;
    virtual void contactPointConfirmedCallback(hkpContactPointConfirmedEvent& event);
    void contactPointCallback(const hkpContactPointEvent& event) override;
    ~hkpCollisionListener() override;
};
