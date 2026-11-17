#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomBroadPhaseListener.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

// When either handle belongs to a phantom, the other handle's collidable is added to that phantom's overlaps, A's
// phantom first. The owners are Havok's hkAddByteOffset of the object's address and its signed byte m_ownerOffset
// (untyped address arithmetic, hence the reinterpret_casts through uint8_t*).
void hkpPhantomBroadPhaseListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        // EXE-INLINED 0x4B59D0
        hkpPhantom* pPhantom =
            reinterpret_cast<hkpPhantom*>(reinterpret_cast<uint8_t*>(pCollidableA) + pCollidableA->m_ownerOffset);
        pPhantom->addOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        // EXE-INLINED 0x4B59D0
        hkpPhantom* pPhantom =
            reinterpret_cast<hkpPhantom*>(reinterpret_cast<uint8_t*>(pCollidableB) + pCollidableB->m_ownerOffset);
        pPhantom->addOverlappingCollidable(pCollidableA);
    }
}

// The same as addCollisionPair, removing the other collidable from the phantom's overlaps.
void hkpPhantomBroadPhaseListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
    hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
    if (pHandleA->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        // EXE-INLINED 0x4B59D0
        hkpPhantom* pPhantom =
            reinterpret_cast<hkpPhantom*>(reinterpret_cast<uint8_t*>(pCollidableA) + pCollidableA->m_ownerOffset);
        pPhantom->removeOverlappingCollidable(pCollidableB);
    }
    hkpTypedBroadPhaseHandle* pHandleB = pair.getElementB();
    if (pHandleB->m_type == hkpWorldObject::BROAD_PHASE_PHANTOM) {
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableB =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleB) + pHandleB->m_ownerOffset);
        hkpTypedBroadPhaseHandle* pHandleA = pair.getElementA();
        // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
        hkpCollidable* pCollidableA =
            reinterpret_cast<hkpCollidable*>(reinterpret_cast<uint8_t*>(pHandleA) + pHandleA->m_ownerOffset);
        // EXE-INLINED 0x4B59D0
        hkpPhantom* pPhantom =
            reinterpret_cast<hkpPhantom*>(reinterpret_cast<uint8_t*>(pCollidableB) + pCollidableB->m_ownerOffset);
        pPhantom->removeOverlappingCollidable(pCollidableA);
    }
}
