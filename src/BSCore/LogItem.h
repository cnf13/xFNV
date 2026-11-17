#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// One entry of the heaps' allocation log: the source line and file of the allocation, the memory context it was
// made under, the stack trace, the heap's counters at that moment, the log chain's links and the entry's lock. The
// PDB's struct LogItem; its members up to the lock are the PDB's, at the PDB's offsets. The PDB's Xbox record is 0x58
// bytes, whose BSSpinLock is 8; the PC BSSpinLock is 0x20 (BSCore/BSSpinLock.h), so the record is 0x70 here.
// AbstractHeap::GetLogIndex (0xAAA560) counts the entries reachable from an entry through its second link, +0x4C.
struct LogItem {
	int32_t iLine;
	const char* pFullPath;
	MEM_CONTEXT eMemContext;
	int32_t iNumStackTraces;
	void* piStackTrace[8];
	uint32_t AllocCounter;
	uint32_t FreeCounter;
	uint32_t AllocTotal;
	uint32_t FreeTotal;
	uint32_t LargestAllocation;
	uint32_t iLastTimeMS;
	LogItem* pNext;
	LogItem* pPrev;
	BSSpinLock Lock;
};
