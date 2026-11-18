// Stand-ins for BSCore/MemoryManager.h's allocation functions: the C runtime's heap.
#include "BSCore/MemoryManager.h"

#include <cstdlib>
#include <cstring>

void* ourMalloc(uint32_t aSize) {
    return std::malloc(aSize);
}

void* ourCalloc(uint32_t aNum, uint32_t aSize) {
    return std::calloc(aNum, aSize);
}

void ourFree(void* apPtr) {
    std::free(apPtr);
}

void* ourMemset(void* apDest, int32_t aiValue, uint32_t aCount) {
    return std::memset(apDest, aiValue, aCount);
}

void* ourMemcpy(void* apDest, const void* apSrc, size_t aCount) {
    return std::memmove(apDest, apSrc, aCount);
}
