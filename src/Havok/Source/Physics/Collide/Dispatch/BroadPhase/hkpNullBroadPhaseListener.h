#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"

// The broad phase listener that ignores every pair; hkpTypedBroadPhaseDispatcher keeps one for the handle type pairs
// without a listener.
class hkpNullBroadPhaseListener : public hkReferencedObject, public hkpBroadPhaseListener {
public:
    ~hkpNullBroadPhaseListener() override;

    // hkpBroadPhaseListener's vtable at +0x8.
    void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
    // hkpBroadPhaseListener's vtable at +0x8.
    void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

    void addCollisionPair_00D554E0(hkpTypedBroadPhaseHandlePair& pair);
    void removeCollisionPair_00D554F0(hkpTypedBroadPhaseHandlePair& pair);

    // The free is hkReferencedObject's; hkpBroadPhaseListener declares one too.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
};
