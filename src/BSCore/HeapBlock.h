#pragma once

#include "Reconstruction.hpp"

struct LogItem;

// Header of a block in the engine's block heaps.
class HeapBlock {
public:
	// The PDB's empty HeapBlock::HeapBlock() (BSCore\HeapBlocks.cpp, beside Init): the exe's
	// copy only returns this (thiscall, ret 0, 0xAAA6D0), right before Init 0xAAA6E0.
	HeapBlock();

	// Resets the block to an unlinked block of the given size with no flags.
	void Init(int32_t aiSize, LogItem* apLogItem);

	HeapBlock* pPrevious;
	uint32_t iMemSize;
};
