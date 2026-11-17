#include "Havok/Source/Physics/Dynamics/World/Util/BroadPhase/hkpBroadPhaseBorderListener.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

// A pair with one border phantom: the phantom gets the other collidable as an overlap. A handle's owner is the
// collidable m_ownerOffset bytes from the handle, raw byte arithmetic as getOwner does it.
void hkpBroadPhaseBorderListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
            return;
        }
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleA2 = pair.getElementA();
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA2) + pHandleA2->m_ownerOffset);
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the border's overlaps; the owners are found by raw
// byte offsets the same way.
void hkpBroadPhaseBorderListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
            return;
        }
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableA->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_BORDER) {
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleA2 = pair.getElementA();
        // EXE-INLINED 0x9B2C90
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA2) + pHandleA2->m_ownerOffset);
        hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidableB->getOwner());
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}
