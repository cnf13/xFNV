#include "BSCore/MemoryHeap.h"

#include "BSCore/AbstractHeap.h"
#include "BSCore/IMemoryManagerFile.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Memory.h"

MemoryHeap::MemoryHeap(uint32_t aiSize, uint32_t aiInitialSize, const char* apName, bool abSupportsSwapping)
  : AbstractHeap(aiSize, aiInitialSize, apName, 0x10000u), bSupportsSwapping(abSupportsSwapping), bDeletingHeap(false) {
    InitHeap();
}

MemoryHeap::~MemoryHeap() {
    DeleteHeap();
    bDeletingHeap = true;
}

void* MemoryHeap::Allocate(uint32_t aSize, uint32_t aiAlignment) {
    return BaseAllocate(aSize);
}

void MemoryHeap::Deallocate(void* apPointer, uint32_t aiUnused) {
    if (apPointer) {
        BaseFree(apPointer);
    }
}

void MemoryHeap::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) {
    AbstractHeap::GetHeapStats(apStats, !bSupportsSwapping && abFullBlockInfo);
}

int32_t MemoryHeap::OutputBucketInfo(IMemoryManagerFile* apFile) {
    int32_t v3;
    int32_t v4 = 0;
    if (!bSupportsSwapping) {
        return AbstractHeap::OutputBucketInfo(apFile);
    }
    v3 = 0;
    return apFile->Write(&v3, 4u);
}

int32_t MemoryHeap::OutputBlockInfo(IMemoryManagerFile* apFile, bool abOutputWalk) {
    int32_t v4;
    int32_t v5 = 0;
    if (!bSupportsSwapping) {
        return AbstractHeap::OutputBlockInfo(apFile, abOutputWalk);
    }
    v4 = 0;
    return apFile->Write(&v4, 4u);
}

uint32_t MemoryHeap::Size(const void* apPointer, uint32_t aiUnused) {
    // The block header right before an allocation holds its size in the low 28 bits and flags above.
    const uint32_t uiSizeAndFlags = (static_cast<const HeapBlock*>(apPointer) - 1)->iMemSize;
    return uiSizeAndFlags & 0x0FFFFFFF;
}

uint32_t MemoryHeap::TotalSize(const void* apPointer) {
    return ((static_cast<const HeapBlock*>(apPointer) - 1)->iMemSize & 268435455) + sizeof(HeapBlock);
}

// Reserves the whole heap (VirtualAlloc MEM_RESERVE), commits its initial size (MEM_COMMIT) and, when that commit
// fails, releases the reservation (MEM_RELEASE).
void* MemoryHeap::DoHeapAllocation(uint32_t aiSize, uint32_t aiInitialSize) {
    void* lpAddress = platform::Reserve(aiSize);
    if (!lpAddress) {
        return lpAddress;
    }
    if (platform::Commit(lpAddress, aiInitialSize)) {
        return lpAddress;
    }
    platform::Release(lpAddress);
    return nullptr;
}

void MemoryHeap::DoHeapFree(void* apPtr) {
    platform::Release(apPtr);
}

uint32_t MemoryHeap::CreateMorePages(void* apMem, uint32_t aiCurrentSize, uint32_t aiRequestedBytes) {
    uint32_t aiRequestedBytesa = ~(iPageSize - 1) & iPageSize + aiRequestedBytes;
    if (!platform::Commit(static_cast<char*>(apMem) + aiCurrentSize, aiRequestedBytesa)) {
        return 0;
    }
    return aiRequestedBytesa;
}

uint32_t MemoryHeap::CleanExtraPages(void* apMem, uint32_t aiCurrentSize, uint32_t aiFreeBytes) {
    uint32_t aiFreeBytesa = aiFreeBytes & ~(iPageSize - 1);
    if (aiFreeBytesa < iPageSize) {
        return 0;
    }
    if (!platform::Decommit(static_cast<char*>(apMem) + (aiCurrentSize - aiFreeBytesa), aiFreeBytesa)) {
        return 0;
    }
    return aiFreeBytesa;
}

// Decommits the whole pages inside the free block's data (which starts at its free-list links) and marks it
// decommitted. VirtualFree's result is ignored, as the game does.
void MemoryHeap::DecommitPages(FreeHeapBlock* apBlock) {
    uint32_t uiSize = apBlock->iMemSize & 0xFFFFFFF;
    if (uiSize >= iPageSize) {
        // The block's data is raw memory: it starts at the free-list links and is rounded up to a page boundary as an
        // address.
        char* pData = reinterpret_cast<char*>(&apBlock->pPrevFree);
        char* pPages = reinterpret_cast<char*>(
            (reinterpret_cast<uintptr_t>(pData) + iPageSize - 1) & ~static_cast<uintptr_t>(iPageSize - 1)
        );
        uiSize = (uiSize - static_cast<uint32_t>(pPages - pData)) & ~(iPageSize - 1);
        if (uiSize >= iPageSize) {
            // VirtualFree MEM_DECOMMIT.
            platform::Decommit(pPages, uiSize);
            apBlock->iMemSize |= 0x20000000;
        }
    }
}

int32_t MemoryHeap::GetCalculatedMemoryUsed() {
    return 0;
}

int32_t MemoryHeap::GetReportedMemoryUsed() {
    return 0;
}

// The block header sits right before the user pointer; bit 0x40000000 of its size word marks a freed block.
bool MemoryHeap::BlockIsOk(const void* apPointer) {
    const HeapBlock* pBlock = static_cast<const HeapBlock*>(apPointer) - 1;
    return PointerInHeap(apPointer) && (pBlock->iMemSize & 0x40000000) == 0;
}
