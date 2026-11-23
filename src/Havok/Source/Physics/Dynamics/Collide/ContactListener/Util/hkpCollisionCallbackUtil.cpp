#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpCollisionCallbackUtil.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpEndOfStepCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"

// One user; the end of step util is built by its own constructor.
hkpCollisionCallbackUtil::hkpCollisionCallbackUtil() : m_count(1) {}

// The end of step util goes with its member.
hkpCollisionCallbackUtil::~hkpCollisionCallbackUtil() {}

// A contact constraint's data is the contact manager's own embedded m_contactConstraintData, so the manager comes back
// through getSimpleConstraintContactMgr. The event goes to the world's contact listeners first, then to each body's,
// m_source telling them which side of the collision they are.
void hkpCollisionCallbackUtil::constraintAddedCallback(hkpConstraintInstance* constraint) {
    HK_TIMER_BEGIN("CollLtUtil", nullptr);
    hkpConstraintData* pData = constraint->m_data;
    if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
        hkpSimpleConstraintContactMgr* pMgr = static_cast<hkpSimpleContactConstraintData*>(pData)
                                                  ->getSimpleConstraintContactMgr();
        hkpRigidBody* pBodyA = static_cast<hkpRigidBody*>(constraint->m_entities[0]);
        hkpRigidBody* pBodyB = static_cast<hkpRigidBody*>(constraint->m_entities[1]);
        hkpCollisionEvent event(hkpCollisionEvent::SOURCE_WORLD, pBodyA, pBodyB, pMgr);
        hkpWorldCallbackUtil::fireContactConstraintAddedCallback(static_cast<hkpWorld*>(pBodyA->m_world), event);
        event.m_source = hkpCollisionEvent::SOURCE_A;
        hkpEntityCallbackUtil::fireContactConstraintAddedCallback(pBodyA, event);
        event.m_source = hkpCollisionEvent::SOURCE_B;
        hkpEntityCallbackUtil::fireContactConstraintAddedCallback(pBodyB, event);
    }
    HK_TIMER_END();
}

// The same for a constraint that is leaving the world, with the removed callbacks.
void hkpCollisionCallbackUtil::constraintRemovedCallback(hkpConstraintInstance* constraint) {
    HK_TIMER_BEGIN("CollLfUtil", nullptr);
    hkpConstraintData* pData = constraint->m_data;
    if (pData->getType() == hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
        hkpSimpleConstraintContactMgr* pMgr = static_cast<hkpSimpleContactConstraintData*>(pData)
                                                  ->getSimpleConstraintContactMgr();
        hkpRigidBody* pBodyA = static_cast<hkpRigidBody*>(constraint->m_entities[0]);
        hkpRigidBody* pBodyB = static_cast<hkpRigidBody*>(constraint->m_entities[1]);
        hkpCollisionEvent event(hkpCollisionEvent::SOURCE_WORLD, pBodyA, pBodyB, pMgr);
        hkpWorldCallbackUtil::fireContactConstraintRemovedCallback(static_cast<hkpWorld*>(pBodyA->m_world), event);
        event.m_source = hkpCollisionEvent::SOURCE_A;
        hkpEntityCallbackUtil::fireContactConstraintRemovedCallback(pBodyA, event);
        event.m_source = hkpCollisionEvent::SOURCE_B;
        hkpEntityCallbackUtil::fireContactConstraintRemovedCallback(pBodyB, event);
    }
    HK_TIMER_END();
}

void hkpCollisionCallbackUtil::worldDeletedCallback(hkpWorld* world) {
    hkpConstraintListener* v3;
    m_endOfStepCallbackUtil.removeFromWorld(world);
    if (this == nullptr) {
        v3 = nullptr;
    }
    else {
        v3 = static_cast<hkpConstraintListener*>(this);
    }
    world->removeConstraintListener(v3);
    world->removeWorldDeletionListener(this != nullptr ? static_cast<hkpWorldDeletionListener*>(this) : nullptr);
    removeReference();
}

// Counts one more user of world's callback util, creating and installing it for the first. The listener registrations
// convert the new util to its bases with a null test; the end-of-step util's address is taken without one.
void hkpCollisionCallbackUtil::addCollisionCallbackUtil(hkpWorld* world) {
    hkpCollisionCallbackUtil* pUtil = world->m_collisionCallbackUtil;
    if (pUtil) {
        ++pUtil->m_count;
        return;
    }
    pUtil = new hkpCollisionCallbackUtil;
    world->m_collisionCallbackUtil = pUtil;
    world->addWorldDeletionListener(pUtil);
    world->addConstraintListener(pUtil);
    pUtil->m_endOfStepCallbackUtil.addToWorld(world);
}

// Drops one user of the world's callback util; the last one takes it out of the world, off the world's constraint and
// deletion listeners and releases it. The listener bases are taken through references, so the pointer is adjusted with
// no null test.
void hkpCollisionCallbackUtil::removeCollisionCallbackUtil(hkpWorld* world) {
    hkpCollisionCallbackUtil* util = world->m_collisionCallbackUtil;
    if (--util->m_count == 0) {
        util->m_endOfStepCallbackUtil.removeFromWorld(world);
        world->removeConstraintListener(&static_cast<hkpConstraintListener&>(*util));
        world->removeWorldDeletionListener(&static_cast<hkpWorldDeletionListener&>(*util));
        util->removeReference();
    }
}
