#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomBroadPhaseListener.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

// When either handle belongs to a phantom, the other handle's collidable is added to that phantom's overlaps, A's
// phantom first.
void hkpPhantomBroadPhaseListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the phantom's overlaps.
void hkpPhantomBroadPhaseListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        hkpCollidable* pCollidableB = static_cast<hkpCollidable*>(pHandleB->getOwner());
        hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
        hkpCollidable* pCollidableA = static_cast<hkpCollidable*>(pHandleA->getOwner());
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}
