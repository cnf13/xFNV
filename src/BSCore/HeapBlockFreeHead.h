#pragma once

#include "BSCore/HeapBlock.h"
#include "Reconstruction.hpp"

// A free block of a block heap: the block header, the neighbours in the free list and the list head that points at
// it. IDA's name; the PDB has no such class.
class HeapBlockFreeHead : public HeapBlock {
public:
	HeapBlockFreeHead* pNextFree;
	HeapBlockFreeHead* pPrevFree;
	HeapBlockFreeHead** ppRoot;
};
