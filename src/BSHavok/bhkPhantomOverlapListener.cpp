#include "BSHavok/bhkPhantomOverlapListener.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"

hkpPhantomOverlapListener::~hkpPhantomOverlapListener() {}

void hkpPhantomOverlapListener::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
