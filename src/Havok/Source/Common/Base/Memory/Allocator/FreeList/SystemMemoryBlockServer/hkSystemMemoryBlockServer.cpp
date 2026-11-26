#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/SystemMemoryBlockServer/hkSystemMemoryBlockServer.h"

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A zero minBlockSize takes the default minimum allocation of 128 KiB.
hkSystemMemoryBlockServer::hkSystemMemoryBlockServer(hkMemoryAllocator* system, uint32_t minBlockSize)
  : hkMemoryBlockServer(minBlockSize ? minBlockSize : 131072u), m_allocated(0), m_memoryLimit(0), m_system(system) {}

hkSystemMemoryBlockServer::~hkSystemMemoryBlockServer() {}

// The system's pages are not one single block.
hkBool hkSystemMemoryBlockServer::isSingleBlockServer() {
    return false;
}

uint32_t hkSystemMemoryBlockServer::recommendSize(uint32_t size) {
    uint32_t result;
    int32_t v2 = static_cast<int32_t>(m_memoryLimit);
    if (v2) {
        result = v2 - m_allocated;
        if (result < size) {
            return size;
        }
    }
    else {
        result = m_minAllocSize;
        if (size >= result) {
            return (size + 32767) & 0xFFFF8000u;
        }
    }
    return result;
}

void* hkSystemMemoryBlockServer::allocate(uint32_t size, uint32_t& sizeOut) {
    uint32_t m_memoryLimit;
    m_memoryLimit = this->m_memoryLimit;
    if (m_memoryLimit && ((size + m_allocated) > m_memoryLimit)) {
        return nullptr;
    }
    sizeOut = size;
    m_allocated += size;
    return m_system->blockAlloc(static_cast<int32_t>(size));
}

// The SDK's free(void*, hk_size_t): the size goes on as blockFree's int.
void hkSystemMemoryBlockServer::ourFree(void* apMemory, uint32_t aSize) {
    m_allocated -= aSize;
    m_system->blockFree(apMemory, static_cast<int32_t>(aSize));
}

hkBool hkSystemMemoryBlockServer::resize(void* data, uint32_t oldSize, uint32_t newSize, uint32_t& sizeOut) {
    hkBool result;
    result.m_bool = 0;
    return result;
}

uint32_t hkSystemMemoryBlockServer::getTotalAvailableMemory() {
    int32_t v1 = static_cast<int32_t>(m_memoryLimit);
    if (v1) {
        return v1 - m_allocated;
    }

    return static_cast<uint32_t>(-1);
}

uint32_t hkSystemMemoryBlockServer::getMemoryLimit() {
    return m_memoryLimit;
}

hkBool hkSystemMemoryBlockServer::setMemoryLimit(uint32_t size) {
    hkBool result;
    if (size) {
        if (m_allocated <= size) {
            m_memoryLimit = size;
            result.m_bool = 1;
            return result;
        }

        result.m_bool = 0;
        return result;
    }

    m_memoryLimit = 0;
    result.m_bool = 1;
    return result;
}

// The SDK's criticalAlloc(hk_size_t): the size goes on as blockAlloc's int.
void* hkSystemMemoryBlockServer::criticalAlloc(uint32_t size) {
    return m_system->blockAlloc(static_cast<int32_t>(size));
}

// The SDK's criticalFree(void*, hk_size_t): the size goes on as blockFree's int.
void hkSystemMemoryBlockServer::criticalFree(void* data, uint32_t size) {
    m_system->blockFree(data, static_cast<int32_t>(size));
}
