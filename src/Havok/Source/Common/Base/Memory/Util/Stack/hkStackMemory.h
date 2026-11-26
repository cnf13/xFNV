#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A thread's Havok stack allocator: the next free byte and the bounds of the block it hands out of.
struct hkStackMemory {
    // The bytes left in the block.
    int32_t getFreeStackSize();
    // Hands out the next numBytes bytes of the block; what names the use (unused here).
    void* allocateStack(int32_t numBytes, const char* what);
    // Hands out numBytes rounded up to a multiple of 128.
    void* allocateStackRoundSizeTo128(int32_t numBytes, const char* what) {
        return allocateStack((numBytes + 127) & ~127, what);
    }
    void deallocateStack(void* p);
    // Gives back the bytes past the first newSize of the last block, which starts at p.
    void shrinkAllocatedStack(void* p, int32_t newSize) {
        m_next = static_cast<char*>(p) + newSize;
    }
    void initMemory(void* p, int32_t size);

    // The calling thread's stack memory: its router's m_stack, at the router's start.
    static hkStackMemory& getInstance();

    hkPadSpu<char*> m_next;
    hkPadSpu<void*> m_start;
    hkPadSpu<void*> m_end;
};

// Checks the free size (the assert's call, the result unused), then moves m_next past the block.
inline void* hkStackMemory::allocateStack(int32_t numBytes, const char* what) {
    getFreeStackSize();
    char* pCurrent = m_next;
    m_next = hkAddByteOffset(pCurrent, numBytes);
    return pCurrent;
}

inline void hkStackMemory::deallocateStack(void* p) {
    m_next = static_cast<char*>(p);
}

inline void hkStackMemory::initMemory(void* p, int32_t size) {
    m_start.m_storage = p;
    m_next = static_cast<char*>(p);
    m_end.m_storage = hkAddByteOffset<char>(static_cast<char*>(p), size);
}

// Returns the bytes from m_next to m_end, reading m_end first.
inline int32_t hkStackMemory::getFreeStackSize() {
    void* pEnd = m_end;
    char* pNext = m_next;
    return hkGetByteOffset(pNext, pEnd);
}
