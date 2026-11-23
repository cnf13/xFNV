#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStorageStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

hkRelocationInfo::hkRelocationInfo() : m_pool(nullptr) {}

// The pool's class has no destructor of its own here: its destructor (clear, then its map's) and its operator delete
// (the heap's blockFree) are written out.
hkRelocationInfo::~hkRelocationInfo() {
    if (m_pool) {
        m_pool->clear();
        m_pool->~hkStorageStringMap();
        hkMemoryRouter::getInstance().m_heap->blockFree(
            m_pool,
            sizeof(hkStorageStringMap<int32_t, hkContainerHeapAllocator>)
        );
    }
}

// The map keys are hkUlong words holding the string pointers, hence the pointer/integer reinterpret_casts.
void hkRelocationInfo::addImport(int32_t off, const char* name) {
    if (!m_pool) {
        m_pool = new hkStorageStringMap<int32_t, hkContainerHeapAllocator>;
    }
    const char* pcName = m_pool->insert(name, 0);
    m_imports.pushBack(Import{off, pcName});
}

// Each local fixup stores the buffer address of its target, each global one its target's address, at its offset in the
// buffer. The buffer is raw packfile data and untyped: the fixup slots are pointers written through casts.
void hkRelocationInfo::applyLocalAndGlobal(void* buffer) {
    char* pBuffer = static_cast<char*>(buffer);
    for (const Local& kLocal : m_local) {
        *reinterpret_cast<void**>(pBuffer + kLocal.m_fromOffset) = pBuffer + kLocal.m_toOffset;
    }
    for (const Global& kGlobal : m_global) {
        *reinterpret_cast<void**>(pBuffer + kGlobal.m_fromOffset) = kGlobal.m_toAddress;
    }
}
