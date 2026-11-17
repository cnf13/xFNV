#pragma once

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

#include <stdint.h>

// Havok's allocator adapter to the game's memory manager; it adds no per-instance state.
class bhkMemoryAllocator : public hkMemoryAllocator {
public:
	~bhkMemoryAllocator() override;

	void* blockAlloc(int32_t nbytes) override;
	void blockFree(void* p, int32_t __formal) override;
	void getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& __formal) override {}
	int32_t getAllocatedSize(const void* __formal, int32_t nbytes) override;
};
