#pragma once

#include <cstdint>

class hkpBroadPhaseHandle;

// The interface a broad phase cast reports each handle it reaches to; the result is the cast's new early out
// fraction. It holds no state.
class hkpBroadPhaseCastCollector {
public:
    virtual ~hkpBroadPhaseCastCollector() {}

    // Called for each handle a cast reaches; returns the new hit fraction of cast castIndex.
    virtual float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) = 0;

    // DeletingDestruct's free (MemoryManager::Deallocate).
    static void operator delete(void* apMemory);
};
