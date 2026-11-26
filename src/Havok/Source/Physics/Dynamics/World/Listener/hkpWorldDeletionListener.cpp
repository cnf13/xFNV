#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"

#include "BSCore/NewOverloads.h"

hkpWorldDeletionListener::~hkpWorldDeletionListener() {}

void hkpWorldDeletionListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
