#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"

// The reflection data of hkWorldMemoryAvailableWatchDog (the PDB's hkWorldMemoryAvailableWatchDogClass.obj; the SDK
// has no .cpp here to name the file, so it takes the header's stem): the class record, no reflected members, version 1.
hkClass hkWorldMemoryAvailableWatchDogClass("hkWorldMemoryAvailableWatchDog", &hkReferencedObjectClass, 8, nullptr, 0,
	nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 1);

// The exe has no copy (no vtable of the abstract class): its subclasses' destructors inline it, with nothing to free.
hkWorldMemoryAvailableWatchDog::~hkWorldMemoryAvailableWatchDog() {}
