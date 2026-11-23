#pragma once

#include <cstdint>

struct LogItem;

// Header of a block in the engine's block heaps.
class HeapBlock {
public:
    // iMemSize holds the block's size in its low 28 bits and these flags above them.
    enum : uint32_t {
        SIZE_MASK = 0x0FFFFFFF,
        DECOMMITTED = 0x20000000,
        // The block is on a free list.
        IS_FREE = 0x40000000,
    };

    HeapBlock();

    // Resets the block to an unlinked block of the given size with no flags.
    void Init(int32_t aiSize, LogItem* apLogItem);

    HeapBlock* pPrevious;
    uint32_t iMemSize;
};
