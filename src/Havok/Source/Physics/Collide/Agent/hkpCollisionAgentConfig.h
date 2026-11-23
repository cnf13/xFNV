#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <cstdint>

// The world's settings for the collision agents' iterative linear casts.
class hkpCollisionAgentConfig {
public:
    hkpCollisionAgentConfig() : m_iterativeLinearCastEarlyOutDistance(0.01f), m_iterativeLinearCastMaxIterations(20) {}

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }
    static void* operator new(size_t, void* apMemory) noexcept {
        return apMemory;
    }
    static void operator delete(void*, void*) noexcept {}

    float m_iterativeLinearCastEarlyOutDistance;
    int32_t m_iterativeLinearCastMaxIterations;
    uint8_t padding08[8];
};
