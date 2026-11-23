#include "Havok/Source/Physics/Dynamics/World/Util/BroadPhase/hkpBroadPhaseBorderListener.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

// A pair with one border phantom: the phantom gets the other collidable as an overlap.
void hkpBroadPhaseBorderListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
            return;
        }
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpTypedBroadPhaseHandle* pHandleA2 = pair.getElementA();
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA2->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the border's overlaps.
void hkpBroadPhaseBorderListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
            return;
        }
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpTypedBroadPhaseHandle* pHandleA2 = pair.getElementA();
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA2->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}
