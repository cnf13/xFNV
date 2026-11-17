#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Reconstruction.hpp"

// The broad phase listener that ignores every pair; hkpTypedBroadPhaseDispatcher keeps one for the handle type pairs
// without a listener. Its code stays with the exe. IDA sizes it 0x8 (an empty hkpBroadPhaseListener); the PDB gives
// 12, and hkpWorld::hkpWorld allocates 268 bytes for the hkpTypedBroadPhaseDispatcher that embeds it.
class hkpNullBroadPhaseListener : public hkReferencedObject, public hkpBroadPhaseListener {
public:
	~hkpNullBroadPhaseListener() override;

	// hkpBroadPhaseListener's vtable at +0x8.
	void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;
	// hkpBroadPhaseListener's vtable at +0x8.
	void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) override;

	void addCollisionPair_00D554E0(hkpTypedBroadPhaseHandlePair& pair);
	void removeCollisionPair_00D554F0(hkpTypedBroadPhaseHandlePair& pair);

	// The free of the exe's slot 0 (0xC98370): hkReferencedObject's; hkpBroadPhaseListener declares one too.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
};
inline constexpr fnvd::Vtable hkpNullBroadPhaseListener_vftable_hkpBroadPhaseListener{};
inline constexpr fnvd::Vtable hkpNullBroadPhaseListener_vftable{};
