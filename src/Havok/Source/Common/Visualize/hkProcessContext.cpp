#include "Havok/Source/Common/Visualize/hkProcessContext.h"

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// The two inplace arrays free their buffers in their own destructors.
hkProcessContext::~hkProcessContext() {}

void hkProcessContext::operator delete(void* apMemory) {
    ourFree(apMemory);
}
