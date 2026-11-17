#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A thread's Havok stack allocator: the next free byte and the bounds of the block it hands out of.
struct hkStackMemory {
    // The bytes left in the block.
    int32_t getFreeStackSize();
    // Hands out the next numBytes bytes of the block; what names the use (unused here).
    void* allocateStack(int32_t numBytes, const char* what);
    void deallocateStack(void* p);
    void initMemory(void* p, int32_t size);

    // The calling thread's stack memory: its router's m_stack, at the router's start.
    static hkStackMemory& getInstance();

    hkPadSpu<char*> m_next;
    hkPadSpu<void*> m_start;
    hkPadSpu<void*> m_end;
};
