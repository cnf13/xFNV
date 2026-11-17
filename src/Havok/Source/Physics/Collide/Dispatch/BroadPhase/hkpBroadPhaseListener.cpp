#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"

#include "BSCore/MemoryManager.h"

hkpBroadPhaseListener::~hkpBroadPhaseListener() {}

void hkpBroadPhaseListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}
