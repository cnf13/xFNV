#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Reconstruction.hpp"

class hkMemoryAllocator;

// The allocator policy of Havok's containers that allocate from the calling thread's heap: an empty class the
// containers construct as a temporary to fetch the allocator.
struct hkContainerHeapAllocator {
	// The calling thread's heap allocator; the argument is unused (the SDK's inline body). The exe's out-of-line copy
	// (thiscall, ret 4, 0x4A4840) is registered in BGSDecalManager.cpp.
	hkMemoryAllocator& get(const void*) { return hkMemoryRouter::getInstance().heap(); }
};

// The allocator policy of Havok's containers that allocate from the calling thread's temp allocator
// (hkMemoryRouter::getInstance().temp()): an empty class like hkContainerHeapAllocator (PC PDB: size 1). The exe
// inlines its get (e.g. hkStringBuf::split 0xD61910 reads the router's m_temp), so nothing is declared.
struct hkContainerTempAllocator {
	// The calling thread's temp allocator; the argument is unused (the SDK's inline body).
	hkMemoryAllocator& get(const void*) { return hkMemoryRouter::getInstance().temp(); }
};

// The allocator policy of Havok's containers that allocate from the calling thread's debug allocator
// (hkMemoryRouter::getInstance().debug()): an empty class like hkContainerHeapAllocator (hkbase.pdb: size 1). The
// exe has no out-of-line copy of its get, so nothing is declared.
struct hkContainerDebugAllocator {
	// The calling thread's debug allocator; the argument is unused (the SDK's inline body).
	hkMemoryAllocator& get(const void*) { return hkMemoryRouter::getInstance().debug(); }
};
