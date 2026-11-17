#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/System/StackTracer/hkStackTracer.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

class hkDebugMemorySnapshot;

// An allocator that records every live allocation (with its call stack) to report leaks. Only some of its members are
// declared.
class hkLeakDetectAllocator : public hkMemoryAllocator {
public:
	// The record of one live allocation.
	struct AllocInfo {
		int32_t traceId;
		int32_t size;
		hkBool isBuf;
	};

	struct Dynamic;

	hkLeakDetectAllocator();

	void* blockAlloc(int32_t numBytes) override;
	void blockFree(void* p, int32_t numBytes) override;
	void* bufAlloc(int32_t& reqNumInOut) override;
	void bufFree(void* p, int32_t num) override;
	void* bufRealloc(void* pold, int32_t oldNum, int32_t& reqNumInOut) override;
	void getMemoryStatistics(hkMemoryAllocator::MemoryStatistics& u) override;
	int32_t getAllocatedSize(const void* obj, int32_t numBytes) override;
	~hkLeakDetectAllocator() override;

protected:
	// Allocates aiNumBytes from the child allocator and records it; aiIsBuf is 1 for the buffer interface, 0 for
	// blocks.
	void* checkedAlloc(int32_t isBuf, int32_t numBytes);
	// Checks the record of apMem, forgets it and frees it.
	void checkedFree(int32_t isBuf, void* p, int32_t numBytes);

public:
	// Reports message about the allocation at ptr with its record info: the address and size, then info's call stack.
	void danger(const char* message, const void* ptr, const AllocInfo& info) const;
	// Fills snapshot with every live allocation and its call stack.
	hkResult getMemorySnapshot(hkDebugMemorySnapshot& snapshot);
	// Reports the leaks left and frees the allocator's own state.
	void quit();
	void init(hkMemoryAllocator* child, hkMemoryAllocator* debug, void (*output)(const char*, void*),
		void* outputUserData);

	hkStackTracer* m_stackTracer;
	hkStackTracer::CallTree m_callTree;
	hkCriticalSection m_criticalSection;
	hkMemoryAllocator* m_childAllocator;
	hkMemoryAllocator* m_debugAllocator;
	void (*m_outputFunc)(const char* str, void* userData);
	void* m_outputFuncArg;
	Dynamic* m_dynamic;
	uint32_t m_inUse;
};

// The map from a live allocation's address to its record, keyed by hkUlong. Its home is here: AllocInfo is
// hkLeakDetectAllocator's nested type, which hkMap.h cannot see.
template <> class hkMapBase<hkUlong, hkLeakDetectAllocator::AllocInfo, hkMapOperations<hkUlong>> {
public:
	enum : int32_t {
		NUM_ELEMS_MASK = 0x7fffffff,
		DONT_DEALLOCATE_FLAG = INT32_MIN,
	};

	class Dummy;
	// A slot index, opaque to the map's users (the SDK's).
	using Iterator = Dummy*;

	// The address, then its record; the key -1 marks an empty slot.
	struct Pair {
		hkUlong key;
		hkLeakDetectAllocator::AllocInfo val;
	};

	// Adds key with val, or replaces its value, growing the table from alloc first when it is half full; returns 1 when
	// the key is new (hkBool32).
	int32_t insert(hkMemoryAllocator& alloc, hkUlong key, hkLeakDetectAllocator::AllocInfo val);
	// The slot of key, or m_hashMod + 1 when it is absent.
	Iterator findKey(hkUlong key) const;
	// Empties slot it and moves the following pairs of its probe run back.
	void remove(Iterator it);
	// Empties the map, then frees the table through alloc unless the map does not own it.
	void clearAndDeallocate(hkMemoryAllocator& alloc);

protected:
	// Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs again; the old storage goes
	// back to alloc unless the map does not own it.
	void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);

public:
	Pair* m_elem;
	int32_t m_numElems;
	int32_t m_hashMod;
};

using hkLeakDetectAllocatorMap = hkMapBase<hkUlong, hkLeakDetectAllocator::AllocInfo, hkMapOperations<hkUlong>>;

// The allocator's live-allocation table; m_dynamic points here, and the map's members take it as this (checkedAlloc
// passes m_dynamic to insert).
struct hkLeakDetectAllocator::Dynamic {
	hkLeakDetectAllocatorMap m_activePointers;
};
