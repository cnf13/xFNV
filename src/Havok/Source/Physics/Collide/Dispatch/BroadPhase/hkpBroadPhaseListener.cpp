#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"

#include "BSCore/NewOverloads.h"

hkpBroadPhaseListener::~hkpBroadPhaseListener() {}

void hkpBroadPhaseListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
