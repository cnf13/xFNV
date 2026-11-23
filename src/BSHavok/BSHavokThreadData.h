#pragma once

#include <cstdint>

class hkMemoryRouter;

// Havok memory state for an engine thread.
struct BSHavokThreadData {
    hkMemoryRouter* pHavokMemoryRouter;
    void* pHavokAlloc;
    uint32_t uiOwnerID;
};
