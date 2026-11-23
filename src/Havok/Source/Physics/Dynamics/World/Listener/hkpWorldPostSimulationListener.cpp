#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

hkpWorldPostSimulationListener::~hkpWorldPostSimulationListener() {}

void hkpWorldPostSimulationListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
