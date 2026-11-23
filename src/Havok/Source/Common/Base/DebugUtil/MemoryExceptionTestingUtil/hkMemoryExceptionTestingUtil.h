#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <stdint.h>

class hkMemoryExceptionTestingUtil;

// Tracks frame-local memory exception injection checks.
class hkMemoryExceptionTestingUtil {
public:
    enum : int32_t {
        MAX_CHECK_ID = 0x32,
    };

    hkMemoryExceptionTestingUtil();
    ~hkMemoryExceptionTestingUtil();

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }
    static void* operator new(size_t, void* apMemory) {
        return apMemory;
    }
    static void operator delete(void*, void*) noexcept {}

    void startFrameImpl();
    void startNewDemoImpl();
    void endFrameImpl();
    void allowMemoryExceptionsImpl(bool allowMemExceptions);
    bool isMemoryAvailableImpl(int32_t id);

    static void destroy();
    static hkMemoryExceptionTestingUtil& getInstance();
    static void create();

    bool m_outOfMemory;
    int32_t m_frameCounter;
    bool m_allowMemoryExceptions;
    bool m_wasCheckIdThrown[50];

    // create stores the object it constructs here and getInstance returns it.
    static inline hkMemoryExceptionTestingUtil* s_instance;
};
