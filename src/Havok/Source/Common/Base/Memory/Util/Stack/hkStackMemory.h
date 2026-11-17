#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// A thread's Havok stack allocator: the next free byte and the bounds of the block it hands out of.
struct hkStackMemory {
	// The bytes left in the block.
	int32_t getFreeStackSize();
	// Hands out the next numBytes bytes of the block; what names the use (unused here).
	void* allocateStack(int32_t numBytes, const char* what);
	void deallocateStack(void* p);
	void initMemory(void* p, int32_t size);

	// The calling thread's stack memory: its router's m_stack, at the router's start, so the exe's copy is a jump to
	// hkMemoryRouter::getInstance (cdecl, ret 0, 0xE4E3F0); 0xE2C8A0 and 0xE2CC30 call it.
	static hkStackMemory& getInstance();

	// The exe reads and writes these through hkPadSpu's out-of-line conversion and assignment (getFreeStackSize
	// 0x5E0B10 converts m_end); callers whose stores the exe inlines write m_storage.
	hkPadSpu<char*> m_next;
	hkPadSpu<void*> m_start;
	hkPadSpu<void*> m_end;
};
