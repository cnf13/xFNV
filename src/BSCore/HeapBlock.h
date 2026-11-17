#pragma once

#include <cstdint>

struct LogItem;

// Header of a block in the engine's block heaps.
class HeapBlock {
public:
    HeapBlock();

    // Resets the block to an unlinked block of the given size with no flags.
    void Init(int32_t aiSize, LogItem* apLogItem);

    HeapBlock* pPrevious;
    uint32_t iMemSize;
};
