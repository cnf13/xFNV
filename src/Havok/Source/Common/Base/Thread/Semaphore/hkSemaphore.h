#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <cstdint>

// The standard-C++ semaphore hkSemaphore keeps in its handle's slot
// (hkWin32Semaphore.cpp).
struct hkSemaphoreHandle;

// A counting semaphore. Not polymorphic.
class hkSemaphore {
public:
    hkSemaphore(int32_t initialCount, int32_t maxCount);
    ~hkSemaphore();

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

    // Waits for the semaphore without a timeout.
    void acquire();
    // Releases the semaphore lReleaseCount times. ReleaseSemaphore's BOOL is not returned because no caller reads it,
    // nor do the callers of the static form that passes it on.
    void release(int32_t lReleaseCount);

    // Releases semaphore count times (the static form hkJobQueue calls).
    static void release(hkSemaphore* semaphore, int32_t count);
    // Waits for semaphore (the static form hkJobQueue calls).
    static void acquire(hkSemaphore* semaphore);

    hkSemaphoreHandle* m_semaphore;
};
