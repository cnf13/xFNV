#pragma once

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/string.h"

#include <cstring>
#include <stddef.h>
#include <stdint.h>

// The engine's CRT free.
inline void ourFree(void* apPtr) {
    MemoryManager::Instance().Deallocate(apPtr);
}

// The engine's CRT malloc.
inline void* ourMalloc(uint32_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

// The engine's CRT realloc.
inline void* ourRealloc(void* apPtr, uint32_t aSize) {
    return MemoryManager::Instance().Reallocate(apPtr, aSize);
}

// The engine's memcpy wrapper: it forwards to the CRT memcpy and returns its result.
inline void* ourMemcpy(void* apDest, const void* apSrc, size_t aCount) {
    return rt::memcpy(apDest, apSrc, aCount);
}

// The engine's memset wrapper (it calls the CRT memset and returns its result).
inline void* ourMemset(void* apDest, int32_t aiValue, uint32_t aCount) {
    return std::memset(apDest, aiValue, aCount);
}

// Unbounded copy of apStrSource into apStrDest; returns apStrDest.
inline char* ourStrcpy(char* apStrDest, const char* apStrSource) {
    return std::strcpy(apStrDest, apStrSource);
}

// The engine's zeroing allocator.
inline void* ourCalloc(uint32_t aNum, uint32_t aSize) {
    uint32_t uiSize = aNum * aSize;
    void* pMemory = MemoryManager::Instance().Allocate(uiSize);
    ourMemset(pMemory, 0, uiSize);
    return pMemory;
}

inline void* simpleMalloc(void* opaque, uint32_t aiNumItems, uint32_t aSize) {
    return MemoryManager::Instance().Allocate(aSize * aiNumItems);
}

inline void simpleFree(void* opaque, void* apPtr) {
    MemoryManager::Instance().Deallocate(apPtr);
}
