#pragma once

#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Reconstruction.hpp"

// What the broad phase tells when a pair of handles begins or ends to overlap. The class has only a vtable; both
// callbacks are pure (the exe's slots hold _purecall's UnimplementedVirtFunc, 0xEC60FA).
class hkpBroadPhaseListener {
public:
	virtual ~hkpBroadPhaseListener();

	virtual void addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) = 0;
	virtual void removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkpBroadPhaseListener_vftable{};
