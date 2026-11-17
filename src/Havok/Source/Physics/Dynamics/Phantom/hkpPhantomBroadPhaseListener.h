#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"

// The world's broad phase listener for the pairs that involve a phantom: a new overlap adds the other collidable to
// the phantom, an ended one removes it. No data besides its bases'; hkpWorld::hkpWorld builds it inline.
class hkpPhantomBroadPhaseListener : public hkReferencedObject, public hkpBroadPhaseListener {
public:
	~hkpPhantomBroadPhaseListener() override;

	// hkpBroadPhaseListener's vtable at +0x8.
	void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
	// hkpBroadPhaseListener's vtable at +0x8.
	void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

	// Frees through hkReferencedObject's deleteObject; hkpBroadPhaseListener declares an operator delete too.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
};
