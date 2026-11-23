#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

class hkMemoryAllocator;

// The allocator policy of Havok's containers that allocate from the calling thread's heap: an empty class the
// containers construct as a temporary to fetch the allocator.
struct hkContainerHeapAllocator {
    // The calling thread's heap allocator; the argument is unused.
    hkMemoryAllocator& get(const void*) {
        return hkMemoryRouter::getInstance().heap();
    }
};

// The allocator policy of Havok's containers that allocate from the calling thread's temp allocator
// (hkMemoryRouter::getInstance().temp()): an empty class like hkContainerHeapAllocator.
struct hkContainerTempAllocator {
    // The calling thread's temp allocator; the argument is unused (the SDK's inline body).
    hkMemoryAllocator& get(const void*) {
        return hkMemoryRouter::getInstance().temp();
    }
};

// The allocator policy of Havok's containers that allocate from the calling thread's debug allocator
// (hkMemoryRouter::getInstance().debug()): an empty class like hkContainerHeapAllocator.
struct hkContainerDebugAllocator {
    // The calling thread's debug allocator; the argument is unused (the SDK's inline body).
    hkMemoryAllocator& get(const void*) {
        return hkMemoryRouter::getInstance().debug();
    }
};
