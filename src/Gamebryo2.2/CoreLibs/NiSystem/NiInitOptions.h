#pragma once

#include <cstdint>

class BSNiAllocator;
class NiAllocator;

// The options Gamebryo is initialised with: the allocator it uses and whether it created that allocator itself.
class NiInitOptions {
public:
    NiInitOptions();
    NiInitOptions(BSNiAllocator* pkAllocator);
    ~NiInitOptions();

    NiAllocator* GetAllocator() const;

    NiAllocator* m_pkAllocator;
    bool m_bAllocatedInternally;
};
