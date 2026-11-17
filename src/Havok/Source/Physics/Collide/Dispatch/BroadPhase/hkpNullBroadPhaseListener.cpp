#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpNullBroadPhaseListener.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"

hkpNullBroadPhaseListener::~hkpNullBroadPhaseListener() {}

void hkpNullBroadPhaseListener::addCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
	;
}

void hkpNullBroadPhaseListener::removeCollisionPair(hkpTypedBroadPhaseHandlePair& pair) {
	;
}

void hkpNullBroadPhaseListener::addCollisionPair_00D554E0(hkpTypedBroadPhaseHandlePair& pair) {
	;
}

void hkpNullBroadPhaseListener::removeCollisionPair_00D554F0(hkpTypedBroadPhaseHandlePair& pair) {
	;
}
