#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Where the free list allocator gets its pages from: an interface over the system's memory (or a fixed buffer). Every
// virtual function is abstract.
class hkMemoryBlockServer {
public:
    enum : int32_t {
        ALIGN = 0x10,
    };

    hkMemoryBlockServer(uint32_t aMinAllocSize);
    virtual ~hkMemoryBlockServer();

    virtual hkBool isSingleBlockServer() = 0;
    virtual uint32_t recommendSize(uint32_t aSize) = 0;
    virtual void* allocate(uint32_t aSize, uint32_t& arSizeOut) = 0;
    virtual void ourFree(void* apMemory, uint32_t aSize) = 0;
    virtual hkBool resize(void* apMemory, uint32_t aOldSize, uint32_t aNewSize, uint32_t& arSizeOut) = 0;
    virtual uint32_t getTotalAvailableMemory() = 0;
    virtual uint32_t getMemoryLimit() = 0;
    virtual hkBool setMemoryLimit(uint32_t aLimit) = 0;
    virtual void* criticalAlloc(uint32_t aSize) = 0;
    virtual void criticalFree(void* apMemory, uint32_t aSize) = 0;

    uint32_t m_minAllocSize;
};
