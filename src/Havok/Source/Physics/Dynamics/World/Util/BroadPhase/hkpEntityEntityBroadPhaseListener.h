#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"

class hkpWorld;

// The broad phase listener of the world's entity-entity pairs: a new overlap creates the pair's agent, an ended one
// removes it.
class hkpEntityEntityBroadPhaseListener : public hkReferencedObject, public hkpBroadPhaseListener {
public:
	// The listener of world.
	hkpEntityEntityBroadPhaseListener(hkpWorld* world);
	~hkpEntityEntityBroadPhaseListener() override;

	void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
	void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

	// Both bases declare one: the Havok heap's block of m_memSizeAndFlags, as hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	// Private in the SDK; public here so that its offset can be pinned.
	hkpWorld* m_world;
};
