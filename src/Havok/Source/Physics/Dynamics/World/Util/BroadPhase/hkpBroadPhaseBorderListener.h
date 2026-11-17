#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"

#include <stdint.h>

// Receives the broadphase collision pairs associated with the world's border.
class hkpBroadPhaseBorderListener : public hkReferencedObject, public hkpBroadPhaseListener {
public:
    ~hkpBroadPhaseBorderListener() override;

    // hkpBroadPhaseListener's vtable at +0x8.
    void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
    // hkpBroadPhaseListener's vtable at +0x8.
    void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

    // Both bases declare one: DeletingDestruct's free, the Havok heap's block of m_memSizeAndFlags.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
};
