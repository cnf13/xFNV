#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSCore/IMemoryHeap.h"

struct LogItem;

// A bump heap that never frees individual allocations.
class ZeroOverheadHeap : public IMemoryHeap {
public:
	ZeroOverheadHeap(uint32_t aiSize, const char* apName);

	~ZeroOverheadHeap() override;

	const char* GetName() override;
	void* Allocate(uint32_t aiSize, uint32_t aiAlignment) override;
	void Deallocate(void* apMem, uint32_t aiUnused) override;
	bool PointerInHeap(const void* apPointer) override;
	void CheckHeapIntegrity() override {}
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
	int32_t OutputBucketInfo(IMemoryManagerFile* apFile) override;
	int32_t OutputBlockInfo(IMemoryManagerFile* apFile, bool abFullWalk) override;
	uint32_t Size(const void* apMem, uint32_t aiUnused) override;
	uint32_t TotalSize(const void* apMem) override;
	virtual LogItem* GetLogItem(const void*) { return nullptr; }
	virtual void SetLogItem(void*, LogItem*) {}
	virtual uint32_t GetFullContext(void*) { return 0; }
	virtual void SetFullContext(void*, uint32_t) {}

	uint32_t iSize;
	const char* pName;
	char* pHeap;
	char* pCurrentFree;
	uint32_t iAllocations;
	// Never written.
	uint8_t unk18[8];
	// The heap's lock: Allocate takes it, the constructor clears its owner and count
	// and Allocate releases it inline.
	BSSpinLock CriticalSectionO;
};
