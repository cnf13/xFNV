#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

hkDataRefCounted::~hkDataRefCounted() {}

void hkDataRefCounted::operator delete(void* apMemory) {
    hkDataRefCounted* pThis = static_cast<hkDataRefCounted*>(apMemory);
    hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, pThis->m_memSize);
}

// The interfaces have no members of their own to destroy.
hkDataClassImpl::~hkDataClassImpl() {}

hkDataObjectImpl::~hkDataObjectImpl() {}

hkDataArrayImpl::~hkDataArrayImpl() {}
