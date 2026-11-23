#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "SDK/Windows/winbase.h"

#include <cstddef>

// Havok's critical section: the engine's RTL_CRITICAL_SECTION (0x28 bytes; the exe's is 0x18). Not polymorphic.
class hkCriticalSection {
public:
    hkCriticalSection(int32_t auiSpinCount);
    ~hkCriticalSection();

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

    void enter() {
        EnterCriticalSection(&m_section);
    }
    void leave() {
        LeaveCriticalSection(&m_section);
    }

    // The counter's old value. InterlockedExchangeAdd reads the counter as a signed LONG.
    static uint32_t atomicExchangeAdd(uint32_t* var, int32_t value) {
        return static_cast<uint32_t>(InterlockedExchangeAdd(reinterpret_cast<volatile int32_t*>(var), value));
    }

    _RTL_CRITICAL_SECTION m_section;
};

// A scope's hold on a critical section: the constructor enters m_section, the destructor leaves it.
class hkCriticalSectionLock {
protected:
    hkCriticalSection* m_section;
};
