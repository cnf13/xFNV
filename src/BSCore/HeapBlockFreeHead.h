#pragma once

#include "BSCore/HeapBlock.h"

// A free block of a block heap: the block header, the neighbours in the free list and the list head that points at it.
class HeapBlockFreeHead : public HeapBlock {
public:
    HeapBlockFreeHead* pNextFree;
    HeapBlockFreeHead* pPrevFree;
    HeapBlockFreeHead** ppRoot;
};
