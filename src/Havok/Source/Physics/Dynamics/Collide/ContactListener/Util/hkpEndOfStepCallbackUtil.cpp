#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpEndOfStepCallbackUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// The uids of the two entities of the collision's constraint, the first entity's first, compared lexicographically.
hkBool hkpEndOfStepCallbackUtil::Collision::operator<(const Collision& other) const {
    uint32_t uiUidA = m_mgr->m_constraint.getEntity(0)->m_uid;
    uint32_t uiUidB = m_mgr->m_constraint.getEntity(1)->m_uid;
    uint32_t uiOtherUidA = other.m_mgr->m_constraint.getEntity(0)->m_uid;
    uint32_t uiOtherUidB = other.m_mgr->m_constraint.getEntity(1)->m_uid;
    hkBool kResult;
    kResult.m_bool = (uiUidA < uiOtherUidA) || (uiUidA == uiOtherUidA && uiUidB < uiOtherUidB);
    return kResult;
}

uint64_t hkpEndOfStepCallbackUtil::Collision::getUid() const {
    return (static_cast<uint64_t>(m_mgr->m_constraint.m_entities[0]->getUid()) << 32) |
           m_mgr->m_constraint.m_entities[1]->getUid();
}

hkBool hkpEndOfStepCallbackUtil::NewCollision::operator<(const NewCollision& other) const {
    const uint64_t uid = getUid();
    const uint64_t otherUid = other.getUid();
    return uid < otherUid || (uid == otherUid && m_sequenceNumber < other.m_sequenceNumber);
}

// The contact points of the collision's manifold, last first, each one that is new or every one when the constraint
// asks for contact point callbacks, reported to listener as an end-of-step manifold event.
void hkpEndOfStepCallbackUtil::fireContactPointEventsForCollision(
    hkpSimpleConstraintContactMgr* mgr,
    hkpContactListener* listener,
    hkpCollisionEvent::CallbackSource source
) {
    hkpRigidBody* pBodyA = static_cast<hkpRigidBody*>(mgr->getConstraintInstance()->m_entities[0]);
    hkpRigidBody* pBodyB = static_cast<hkpRigidBody*>(mgr->getConstraintInstance()->m_entities[1]);
    hkpSimpleContactConstraintAtom* pAtom = mgr->m_contactConstraintData.m_atom;
    const int32_t iNumContactPoints = pAtom->m_numContactPoints;
    hkpContactPointPropertiesStream* pProperties = pAtom->getContactPointPropertiesStream(0);
    const int32_t iStriding = pAtom->getContactPointPropertiesStriding();
    hkContactPoint* pContactPoint = pAtom->getContactPoints();
    const bool bFiringFullManifold = (mgr->getConstraintInstance()->getInternal()->m_callbackRequest &
                                      hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK) != 0;
    const int32_t iLast = iNumContactPoints - 1;
    for (int32_t i = iLast; i >= 0; --i) {
        hkpContactPointProperties* pProps = pProperties->asProperties();
        if ((pProps->m_flags & hkContactPointMaterial::CONTACT_IS_NEW) || bFiringFullManifold) {
            hkpContactPointEvent event(source, pBodyA, pBodyB, mgr);
            event.m_type = hkpContactPointEvent::TYPE_MANIFOLD_AT_END_OF_STEP;
            event.m_contactPoint = pContactPoint;
            event.m_contactPointProperties = pProps;
            event.m_firingCallbacksForFullManifold.m_bool = bFiringFullManifold;
            event.m_firstCallbackForFullManifold.m_bool = i == iLast;
            event.m_lastCallbackForFullManifold.m_bool = i == 0;
            event.m_separatingVelocity = nullptr;
            event.m_rotateNormal = nullptr;
            event.m_shapeKeyStorage = pProps->getStartOfExtendedUserData(pAtom);
            event.m_accumulators[0] = nullptr;
            event.m_accumulators[1] = nullptr;
            listener->contactPointCallback(event);
        }
        pProperties = hkAddByteOffset(pProperties, iStriding);
        ++pContactPoint;
    }
}

// Drops from removedCollisions every collision the list also holds (the two cancel out) and copies the rest of the list
// into a new storage, which the list then takes over. That storage is allocated directly through the router's allocator
// rather than through hkArrayUtil::_reserve.
template <typename T>
void hkpEndOfStepCallbackUtil::removeCollisions(hkArray<Collision>& removedCollisions, hkArray<T>& list) {
    if (removedCollisions.m_size == 0) {
        return;
    }
    const int32_t kOriginalRemovedSize = removedCollisions.m_size;
    hkArray<T> filtered(list.m_size);
    if (list.m_size > 0) {
        T* pOut = filtered.m_data;
        for (const T& kEntry : list) {
            int32_t iFound = -1;
            for (uint32_t j = 0; j < removedCollisions.m_size; ++j) {
                if ((removedCollisions.m_data[j] == kEntry).m_bool) {
                    iFound = static_cast<int32_t>(j);
                    break;
                }
            }
            if (iFound < 0) {
                *pOut = kEntry;
                ++pOut;
            }
            else {
                // removeAt: the last collision moves into the freed slot.
                uint32_t uiLast = removedCollisions.m_size - 1;
                removedCollisions.m_size = uiLast;
                if (uiLast != static_cast<uint32_t>(iFound)) {
                    removedCollisions.m_data[iFound] = removedCollisions.m_data[uiLast];
                }
            }
        }
    }
    filtered.setSize(list.m_size + removedCollisions.m_size - kOriginalRemovedSize);
    list.swap(filtered);
}

template void hkpEndOfStepCallbackUtil::removeCollisions<hkpEndOfStepCallbackUtil::NewCollision>(
    hkArray<Collision>& removedCollisions,
    hkArray<NewCollision>& list
);
template void hkpEndOfStepCallbackUtil::removeCollisions<hkpEndOfStepCallbackUtil::Collision>(
    hkArray<Collision>& removedCollisions,
    hkArray<Collision>& list
);

// Merges the new collisions into the registered ones by Collision::operator<, a new one taken only when it orders
// strictly first (the registered one wins a tie), into a storage of exactly the combined size; the collisions take it
// over and the new list empties.
void hkpEndOfStepCallbackUtil::mergeCollisions(hkArray<NewCollision>& newCollisions, hkArray<Collision>& collisions) {
    // Both ends are computed on the addresses and compared as addresses, so that clang does not fold the compares into
    // tests of the counts.
    NewCollision* pNew = newCollisions.m_data;
    const uintptr_t uNewEnd = reinterpret_cast<uintptr_t>(pNew) +
        static_cast<uintptr_t>(newCollisions.m_size) * sizeof(NewCollision);
    if (reinterpret_cast<uintptr_t>(pNew) == uNewEnd) {
        return;
    }
    int32_t numElements = newCollisions.m_size + collisions.m_size;
    hkArray<Collision> merged(numElements);
    Collision* pOut = merged.m_data;
    Collision* pOld = collisions.m_data;
    const uintptr_t uOldEnd = reinterpret_cast<uintptr_t>(pOld) +
        static_cast<uintptr_t>(collisions.m_size) * sizeof(Collision);
    while (reinterpret_cast<uintptr_t>(pNew) < uNewEnd && reinterpret_cast<uintptr_t>(pOld) < uOldEnd) {
        if (!(*pNew < *pOld)) {
            *pOut = *pOld;
            ++pOld;
        }
        else {
            *pOut = *pNew;
            ++pNew;
        }
        ++pOut;
    }
    for (; reinterpret_cast<uintptr_t>(pNew) < uNewEnd; ++pNew) {
        *pOut = *pNew;
        ++pOut;
    }
    for (; reinterpret_cast<uintptr_t>(pOld) < uOldEnd; ++pOld) {
        *pOut = *pOld;
        ++pOut;
    }
    collisions.swap(merged);
    newCollisions.m_size = 0;
}

// Fires the callbacks of the step's collisions. The removed and the re-registered ones cancel out first; with
// deterministic order the new ones are sorted by uid and merged into the registered ones, otherwise they are appended.
// A collision whose master entity sits in an island with the integrate flags set is left to that island's job.
void hkpEndOfStepCallbackUtil::postSimulationCallback(hkpWorld* world) {
    HK_TIMER_BEGIN("EndOfStepCbs", nullptr);
    removeCollisions(m_removedCollisions, m_newCollisions);
    removeCollisions(m_removedCollisions, m_collisions);
    if (m_deterministicOrder.m_bool) {
        hkAlgorithm::quickSort(m_newCollisions.m_data, m_newCollisions.m_size);
        mergeCollisions(m_newCollisions, m_collisions);
    }
    else {
        Collision* pDst = m_collisions.expandBy(m_newCollisions.m_size);
        for (int32_t i = 0; i < m_newCollisions.m_size; ++i) {
            pDst[i].m_mgr = m_newCollisions.m_data[i].m_mgr;
            pDst[i].m_listener = m_newCollisions.m_data[i].m_listener;
            pDst[i].m_source = m_newCollisions.m_data[i].m_source;
        }
        m_newCollisions.m_size = 0;
    }
    for (int32_t i = 0; i < m_collisions.m_size; ++i) {
        Collision& collision = m_collisions.m_data[i];
        hkpSimulationIsland* pIsland = collision.m_mgr->getConstraintInstance()->getSimulationIsland();
        if (pIsland->m_activeMark != 0) {
            fireContactPointEventsForCollision(collision.m_mgr, collision.m_listener, collision.m_source);
        }
    }
    m_sequenceNumber = 0;
    HK_TIMER_END();
}

// The three arrays go with their members.
hkpEndOfStepCallbackUtil::~hkpEndOfStepCallbackUtil() {}

// The same manager, listener and source, stored in apResult.
hkBool hkpEndOfStepCallbackUtil::Collision::operator==(const Collision& other) const {
    return m_mgr == other.m_mgr && m_listener == other.m_listener && m_source == other.m_source;
}

// The three arrays start empty through their own constructors.
hkpEndOfStepCallbackUtil::hkpEndOfStepCallbackUtil() : m_sequenceNumber(0) {
    m_deterministicOrder = false;
}

void hkpEndOfStepCallbackUtil::addToWorld(hkpWorld* world) {
    if (this) {
        world->addWorldPostSimulationListener(this);
    }
    else {
        world->addWorldPostSimulationListener(nullptr);
    }
}

void hkpEndOfStepCallbackUtil::removeFromWorld(hkpWorld* world) {
    if (this) {
        world->removeWorldPostSimulationListener(this);
    }
    else {
        world->removeWorldPostSimulationListener(nullptr);
    }
}

// The Havok SDK's registerCollision: the collision appended to the new ones with the next sequence number.
void hkpEndOfStepCallbackUtil::registerCollision(
    hkpSimpleConstraintContactMgr* mgr,
    hkpContactListener* listener,
    hkpCollisionEvent::CallbackSource source
) {
    hkArray<NewCollision>& newCollisions = m_newCollisions;
    NewCollision& collision = newCollisions.expandOne();
    collision.m_mgr = mgr;
    collision.m_listener = listener;
    collision.m_source = source;
    collision.m_sequenceNumber = m_sequenceNumber;
    m_sequenceNumber++;
}

// The Havok SDK's unregisterCollision: the collision appended to the removed ones.
void hkpEndOfStepCallbackUtil::unregisterCollision(
    hkpSimpleConstraintContactMgr* mgr,
    hkpContactListener* listener,
    hkpCollisionEvent::CallbackSource source
) {
    hkArray<Collision>& removedCollisions = m_removedCollisions;
    Collision& collision = removedCollisions.expandOne();

    collision.m_mgr = mgr;
    collision.m_listener = listener;
    collision.m_source = source;
}
