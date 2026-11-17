#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"

#include <stdint.h>

// One entry of the heaps' allocation log: the source line and file of the allocation, the memory context it was made
// under, the stack trace, the heap's counters at that moment, the log chain's links and the entry's lock.
// AbstractHeap::GetLogIndex counts the entries reachable from an entry through its second link.
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
