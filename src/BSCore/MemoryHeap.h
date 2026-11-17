#pragma once

#include "BSCore/AbstractHeap.h"
#include "Reconstruction.hpp"

// The engine's general heap, a block heap with a header word before every allocation.
class MemoryHeap : public AbstractHeap {
public:
	MemoryHeap(uint32_t aiSize, uint32_t aiInitialSize, const char* apName, bool abSupportsSwapping);

	~MemoryHeap() override;

	void* Allocate(uint32_t aSize, uint32_t aiAlignment) override;
	void Deallocate(void* apPointer, uint32_t aiUnused) override;
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
	int32_t OutputBucketInfo(IMemoryManagerFile* apFile) override;
	int32_t OutputBlockInfo(IMemoryManagerFile* apFile, bool abOutputWalk) override;
	uint32_t Size(const void* apPointer, uint32_t aiUnused) override;
	uint32_t TotalSize(const void* apPointer) override;
	void* DoHeapAllocation(uint32_t aiSize, uint32_t aiInitialSize) override;
	void DoHeapFree(void* apPtr) override;
	uint32_t CreateMorePages(void* apMem, uint32_t aiCurrentSize, uint32_t aiRequestedBytes) override;
	uint32_t CleanExtraPages(void* apMem, uint32_t aiCurrentSize, uint32_t aiFreeBytes) override;
	void DecommitPages(FreeHeapBlock* apBlock) override;

	int32_t GetCalculatedMemoryUsed();
	int32_t GetReportedMemoryUsed();
	bool BlockIsOk(const void* apPointer);

	bool bSupportsSwapping;
	// PDB name; the destructor 0xAA7930 sets it after DestroyHeap.
	bool bDeletingHeap;
};
inline constexpr fnvd::Vtable MemoryHeap_vftable{};
