#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpReportContactMgr.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionData.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Continuous/hkpContinuousSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/Gjk/hkpGskCache.h"

#include <xmmintrin.h>

hkpReportContactMgr::Factory::Factory(hkpWorld* a1) : m_world(a1) {}

hkpContactMgr* hkpReportContactMgr::Factory::createContactMgr(
    const hkpCollidable& a1,
    const hkpCollidable& a2,
    const hkpCollisionInput& a3
) {
    hkpRigidBody* bodyA = static_cast<hkpRigidBody*>(a1.getOwner());
    hkpRigidBody* bodyB = static_cast<hkpRigidBody*>(a2.getOwner());
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpReportContactMgr* v8 =
        static_cast<hkpReportContactMgr*>(Instance->m_heap->blockAlloc(sizeof(hkpReportContactMgr)));
    v8->m_memSizeAndFlags = sizeof(hkpReportContactMgr);
    return ::new (v8) hkpReportContactMgr(m_world, bodyA, bodyB);
}

// The SDK's constructor: the callback delay is the smaller of the two bodies'.
hkpReportContactMgr::hkpReportContactMgr(hkpWorld* sm, hkpRigidBody* bodyA, hkpRigidBody* bodyB)
  : hkpDynamicsContactMgr(hkpContactMgr::TYPE_REPORT_CONTACT_MGR), m_bodyA(bodyA), m_bodyB(bodyB) {
    uint16_t m_contactPointCallbackDelay = bodyB->getContactPointCallbackDelay();
    if (bodyA->getContactPointCallbackDelay() < m_contactPointCallbackDelay) {
        m_contactPointCallbackDelay = bodyA->getContactPointCallbackDelay();
    }
    m_skipNextNprocessCallbacks = m_contactPointCallbackDelay;
    m_world = sm;
}

hkpReportContactMgr::~hkpReportContactMgr() {}

uint16_t hkpReportContactMgr::addContactPointImpl(
    const hkpCdBody& a1,
    const hkpCdBody& a2,
    const hkpProcessCollisionInput& a3,
    hkpProcessCollisionOutput& a4,
    const hkpGskCache* a5,
    hkContactPoint& a6
) {
    hkpRigidBody* m_bodyA = this->m_bodyA;
    hkpRigidBody* m_bodyB = this->m_bodyB;
    __m128 v10 =
        _mm_sub_ps(a6.m_position.m_quad, m_bodyA->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    __m128 v11 =
        _mm_sub_ps(a6.m_position.m_quad, m_bodyB->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1.m_quad);
    __m128 v12 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_add_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(v11, v11, 210),
                        _mm_shuffle_ps(
                            m_bodyB->m_motion.m_angularVelocity.m_quad, m_bodyB->m_motion.m_angularVelocity.m_quad, 201
                        )
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(v11, v11, 201),
                        _mm_shuffle_ps(
                            m_bodyB->m_motion.m_angularVelocity.m_quad, m_bodyB->m_motion.m_angularVelocity.m_quad, 210
                        )
                    )
                ),
                m_bodyB->m_motion.m_linearVelocity.m_quad
            ),
            _mm_add_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(
                            m_bodyA->m_motion.m_angularVelocity.m_quad, m_bodyA->m_motion.m_angularVelocity.m_quad, 201
                        ),
                        _mm_shuffle_ps(v10, v10, 210)
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(
                            m_bodyA->m_motion.m_angularVelocity.m_quad, m_bodyA->m_motion.m_angularVelocity.m_quad, 210
                        ),
                        _mm_shuffle_ps(v10, v10, 201)
                    )
                ),
                m_bodyA->m_motion.m_linearVelocity.m_quad
            )
        ),
        a6.m_separatingNormal.m_quad
    );
    hkpManifoldPointAddedEvent event(
        0, this, &a3, &a4, &a1, &a2, &a6, a5, nullptr,
        _mm_shuffle_ps(v12, v12, 170)[0] + (_mm_shuffle_ps(v12, v12, 85)[0] + v12[0])
    );
    hkpWorldCallbackUtil::fireContactPointAdded(m_world, event);
    if (m_bodyA->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointAddedInternal(m_bodyA, event);
    }
    if (m_bodyB->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointAddedInternal(m_bodyB, event);
    }
    if (event.m_status == HK_CONTACT_POINT_REJECT) {
        removeContactPointImpl(0, *a4.m_constraintOwner.m_storage);
        return -1;
    }

    m_skipNextNprocessCallbacks = event.m_nextProcessCallbackDelay;
    return 0;
}

hkResult hkpReportContactMgr::reserveContactPointsImpl(int32_t) {
    return HK_SUCCESS;
}

// Removes a contact point: the world's listeners are told first, then each entity whose contact listeners are not
// empty. The event carries no properties here, and its m_callbackFiredFrom is left uninitialised like the other events
// of this file.
void hkpReportContactMgr::removeContactPointImpl(uint16_t auiId, hkpConstraintOwner& arOwner) {
    hkpEntity* pEntityA = m_bodyA;
    hkpEntity* pEntityB = m_bodyB;
    hkpContactPointRemovedEvent event;
    event.m_contactPointId = auiId;
    event.m_contactPointProperties = nullptr;
    event.m_entityA = pEntityA;
    event.m_entityB = pEntityB;
    event.m_internalContactMgr = this;
    event.m_constraintOwner = &arOwner;
    hkpWorldCallbackUtil::fireContactPointRemoved(m_world, event);
    // Tests the size of the listeners array before each fire.
    if (pEntityA->m_contactListeners.getSize() > 0) {
        hkpEntityCallbackUtil::fireContactPointRemovedInternal(pEntityA, event);
    }
    if (pEntityB->m_contactListeners.getSize() > 0) {
        hkpEntityCallbackUtil::fireContactPointRemovedInternal(pEntityB, event);
    }
}

// Reports the contact points of a processed collision. The skip counter is decremented and the callback is left out
// while it is still positive; when it runs out it is reloaded with the smaller of the two entities' contact point
// callback delays (the compare is unsigned, on the 16-bit field). The properties array is cleared from the number of
// used contact points down to the first entry: the count is the first-free pointer's index in the collision data's
// array, the byte distance divided by the element size with truncating signed division, not a pointer difference's
// exact one. The event's m_callbackFiredFrom is never written here (the callback util sets it per entity it tells), so
// the local is deliberately left uninitialised, as the game does.
void hkpReportContactMgr::processContactImpl(
    const hkpCollidable& a,
    const hkpCollidable& b,
    const hkpProcessCollisionInput&,
    hkpProcessCollisionData& collisionData
) {
    if (m_skipNextNprocessCallbacks) {
        m_skipNextNprocessCallbacks--;
        return;
    }
    // The owner of a collidable sits m_ownerOffset bytes from it. The owner offset is untyped, so the address is
    // computed and cast.
    hkpEntity* pEntityA =
        const_cast<hkpEntity*>(reinterpret_cast<const hkpEntity*>(reinterpret_cast<const char*>(&a) + a.m_ownerOffset));
    hkpEntity* pEntityB =
        const_cast<hkpEntity*>(reinterpret_cast<const hkpEntity*>(reinterpret_cast<const char*>(&b) + b.m_ownerOffset));
    uint16_t uiCallbackDelay = pEntityA->m_contactPointCallbackDelay;
    if (pEntityB->m_contactPointCallbackDelay < uiCallbackDelay) {
        uiCallbackDelay = pEntityB->m_contactPointCallbackDelay;
    }
    hkpContactProcessEvent event;
    event.m_collidableA = &a;
    event.m_collidableB = &b;
    m_skipNextNprocessCallbacks = uiCallbackDelay;
    event.m_collisionData = &collisionData;
    const ptrdiff_t iByteDistance = reinterpret_cast<const char*>(collisionData.m_firstFreeContactPoint.m_storage) -
        reinterpret_cast<const char*>(collisionData.m_contactPoints);
    const int32_t iNumContactPoints =
        static_cast<int32_t>(iByteDistance / static_cast<ptrdiff_t>(sizeof(hkpProcessCdPoint)));
    for (int32_t i = iNumContactPoints - 1; i >= 0; --i) {
        event.m_contactPointProperties[i] = nullptr;
    }
    event.m_internalContactMgr = this;
    hkpWorldCallbackUtil::fireContactProcess(m_world, event);
    // Tests the size of the listeners array before each fire.
    if (pEntityA->m_contactListeners.getSize() > 0) {
        hkpEntityCallbackUtil::fireContactProcessInternal(pEntityA, event);
    }
    if (pEntityB->m_contactListeners.getSize() > 0) {
        hkpEntityCallbackUtil::fireContactProcessInternal(pEntityB, event);
    }
}

hkpContactMgr::ToiAccept hkpReportContactMgr::addToiImpl(
    const hkpCdBody& a1,
    const hkpCdBody& a2,
    const hkpProcessCollisionInput& a3,
    hkpProcessCollisionOutput& a4,
    float a5,
    hkContactPoint& a6,
    const hkpGskCache* a7,
    float& a8,
    hkpContactPointProperties& a9
) {
    hkpContactPointAddedEvent event;
    event.m_bodyA = &a1;
    event.m_contactPoint = &a6;
    a9.m_friction.m_value = 0;
    a9.m_maxImpulse.m_value = 0;
    a9.m_restitution = 0;
    a9.m_flags = 1;
    hkpRigidBody* m_bodyA = this->m_bodyA;
    event.m_bodyB = &a2;
    double v13 = a8;
    hkpRigidBody* m_bodyB = this->m_bodyB;
    event.m_gskCache = a7;
    event.m_projectedVelocity = v13;
    event.m_collisionInput = &a3;
    event.m_collisionOutput = &a4;
    hkpWorld* m_world = this->m_world;
    event.m_type.m_storage = 0;
    event.m_contactPointProperties = &a9;
    event.m_internalContactMgr = this;
    event.m_status = HK_CONTACT_POINT_ACCEPT;
    hkpEntity* properties = m_bodyB;
    hkpWorldCallbackUtil::fireContactPointAdded(m_world, event);
    if (m_bodyA->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointAddedInternal(m_bodyA, event);
    }
    if (properties->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointAddedInternal(properties, event);
    }
    hkpContactPointAccept result = event.m_status;
    if (event.m_status == HK_CONTACT_POINT_REJECT) {
        removeToiImpl(*a4.m_constraintOwner.m_storage, a9);
        return static_cast<hkpContactMgr::ToiAccept>(event.m_status);
    }

    a8 = event.m_projectedVelocity;

    return static_cast<hkpContactMgr::ToiAccept>(result);
}

void hkpReportContactMgr::removeToiImpl(hkpConstraintOwner& a1, hkpContactPointProperties& a2) {
    hkpContactPointRemovedEvent arEvent;
    hkpRigidBody* m_bodyA = this->m_bodyA;
    hkpWorld* m_world = m_bodyA->m_world;
    arEvent.m_contactPointId = -1;
    hkpRigidBody* m_bodyB = this->m_bodyB;
    arEvent.m_contactPointProperties = &a2;
    arEvent.m_internalContactMgr = this;
    arEvent.m_entityA = m_bodyA;
    arEvent.m_entityB = m_bodyB;
    arEvent.m_constraintOwner = &a1;
    hkpWorldCallbackUtil::fireContactPointRemoved(m_world, arEvent);
    if (m_bodyA->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointRemovedInternal(m_bodyA, arEvent);
    }
    if (m_bodyB->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointRemovedInternal(m_bodyB, arEvent);
    }
}

hkpContactPointProperties* hkpReportContactMgr::getContactPointProperties(uint16_t) {
    return nullptr;
}

hkContactPoint* hkpReportContactMgr::getContactPoint(uint16_t) {
    return nullptr;
}

hkpContactMgr::Type hkpReportContactMgr::getType() const {
    return TYPE_REPORT_CONTACT_MGR;
}

hkpConstraintInstance* hkpReportContactMgr::getConstraintInstance() {
    return nullptr;
}

void hkpReportContactMgr::confirmToi(hkpToiEvent& a1, float a2, hkArray<hkpEntity*, hkContainerHeapAllocator>& a3) {
    hkpContactPointEvent event;
    hkpDynamicsContactMgr* m_contactMgr = a1.m_contactMgr;
    event.m_bodies[1] = static_cast<hkpRigidBody*>(a1.m_entities[1]);
    event.m_contactPoint = &a1.m_contactPoint;
    event.m_contactPointProperties = &a1.m_properties;
    event.m_contactMgr = m_contactMgr;
    hkpRigidBody* v5 = static_cast<hkpRigidBody*>(a1.m_entities[0u]);
    event.m_separatingVelocity = &a1.m_seperatingVelocity;
    event.m_rotateNormal = &a2;
    event.m_source = hkpCollisionEvent::SOURCE_WORLD;
    event.m_bodies[0] = v5;
    event.m_type = hkpContactPointEvent::TYPE_TOI;
    event.m_firingCallbacksForFullManifold.m_bool = 0;
    event.m_firstCallbackForFullManifold.m_bool = 0;
    event.m_lastCallbackForFullManifold.m_bool = 0;
    event.m_shapeKeyStorage = a1.m_extendedUserDatas;
    event.m_accumulators[0] = nullptr;
    event.m_accumulators[1] = nullptr;
    hkpWorldCallbackUtil::fireContactPointCallback(v5->m_world, event);
    hkpEntity* v6 = a1.m_entities[0u];
    event.m_source = hkpCollisionEvent::SOURCE_A;
    if (v6->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointCallbackInternal(v6, event);
    }
    hkpEntity* v7 = a1.m_entities[1];
    event.m_source = hkpCollisionEvent::SOURCE_B;
    if (v7->m_contactListeners.m_size) {
        hkpEntityCallbackUtil::fireContactPointCallbackInternal(v7, event);
    }
}

hkpContactPointAddedEvent::hkpContactPointAddedEvent(
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
)
  : m_bodyA(a), m_bodyB(b), m_contactPoint(cp), m_gskCache(gskCache), m_contactPointProperties(cpp),
    m_projectedVelocity(projectedVelocity), m_status(HK_CONTACT_POINT_ACCEPT), m_internalContactMgr(contactMgr),
    m_collisionInput(collisionInput), m_collisionOutput(collisionOutput) {
    m_type = pointType;
}

hkpManifoldPointAddedEvent::hkpManifoldPointAddedEvent(
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
)
  : hkpContactPointAddedEvent(a2, a3, a4, a5, a6, a7, a8, a9, a10, TYPE_MANIFOLD), m_contactPointId(a1) {
    // The SDK's hkpCollisionEvents.inl: a simple constraint contact manager (type 0) passes on its callback delay.
    if (a2->m_type) {
        m_nextProcessCallbackDelay = 0;
    }
    else {
        m_nextProcessCallbackDelay = static_cast<hkpSimpleConstraintContactMgr*>(a2)->m_contactPointCallbackDelay;
    }
}
