#pragma once

#include "Reconstruction.hpp"

class NiAllocator;

// The engine's memory manager: the allocator every Gamebryo heap request goes through. Only the members
// reconstructed code uses are declared.
class NiMemManager {
public:
	static void _SDMInit();

	NiAllocator* m_pkAllocator;

	static inline NiMemManager* ms_pkMemManager;
};
