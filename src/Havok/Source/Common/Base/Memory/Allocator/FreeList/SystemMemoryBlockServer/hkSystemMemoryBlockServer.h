#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkMemoryBlockServer.h"

class hkMemoryAllocator;

// A block server that takes its pages from another allocator (the system's) up to a memory limit.
class hkSystemMemoryBlockServer : public hkMemoryBlockServer {
public:
    hkSystemMemoryBlockServer(hkMemoryAllocator* a1, uint32_t a2);
    ~hkSystemMemoryBlockServer() override;

    hkBool isSingleBlockServer() override;
    uint32_t recommendSize(uint32_t size) override;
    void* allocate(uint32_t size, uint32_t& sizeOut) override;
    void ourFree(void* apMemory, uint32_t aSize) override;
    hkBool resize(void* data, uint32_t oldSize, uint32_t newSize, uint32_t& sizeOut) override;
    uint32_t getTotalAvailableMemory() override;
    uint32_t getMemoryLimit() override;
    hkBool setMemoryLimit(uint32_t size) override;
    void* criticalAlloc(uint32_t size) override;
    void criticalFree(void* data, uint32_t size) override;

    uint32_t m_allocated;
    // The memory limit.
    uint32_t m_memoryLimit;
    hkMemoryAllocator* m_system;
};
