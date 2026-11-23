#include "Havok/Source/Common/Base/Object/hkReferencedObjectLock.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

// The critical section goes with its own destructor.
hkReferencedObjectLock::~hkReferencedObjectLock() {}
