#pragma once

class NiAllocator;

// The engine's memory manager: the allocator every Gamebryo heap request goes through. Only some of its members are
// declared.
class NiMemManager {
public:
    static void _SDMInit();

    NiAllocator* m_pkAllocator;

    static inline NiMemManager* ms_pkMemManager;
};
