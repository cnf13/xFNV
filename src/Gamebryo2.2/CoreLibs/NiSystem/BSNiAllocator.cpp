#include "Gamebryo2.2/CoreLibs/NiSystem/BSNiAllocator.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "SDK/MSVC/string.h"

BSNiAllocator::BSNiAllocator() {}

// An aligned allocation takes the low byte of stAlignment as the alignment and over-allocates by it; the byte before
// the returned pointer holds the padding length, which Deallocate and Reallocate subtract.
void* BSNiAllocator::Allocate(
    size_t& stSizeInBytes,
    size_t& stAlignment,
    NiMemEventType eEventType,
    int32_t bProvideAccurateSizeOnDeallocate,
    const char* pcFile,
    int32_t iLine,
    const char* pcFunction
) {
    char* pmem;
    if (eEventType == NI_ALIGNEDMALLOC) {
        uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
        char* pcBlock = static_cast<char*>(MemoryManager::Instance().Allocate(stSizeInBytes + ucAlignment));
        // The block is untyped heap memory: the padding comes from its address's low byte.
        uint8_t ucOffset = ucAlignment -
            (static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pcBlock)) & static_cast<uint8_t>(ucAlignment - 1));
        pmem = pcBlock + ucOffset;
        pmem[-1] = static_cast<char>(ucOffset);
    }
    else {
        pmem = static_cast<char*>(MemoryManager::Instance().Allocate(stSizeInBytes, pcFile, iLine, pcFunction));
    }
    std::memset(pmem, 0, stSizeInBytes);
    return pmem;
}

void BSNiAllocator::Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) {
    if (eEventType == NI_ALIGNEDFREE) {
        if (pvMemory) {
            MemoryManager::Instance().Deallocate(
                static_cast<char*>(pvMemory) - static_cast<uint8_t>(*(static_cast<char*>(pvMemory) - 1))
            );
        }
    }
    else {
        MemoryManager::Instance().Deallocate(pvMemory);
    }
}

void* BSNiAllocator::Reallocate(
    void* pvMemory,
    size_t& stSizeInBytes,
    size_t& stAlignment,
    NiMemEventType eEventType,
    int32_t bProvideAccurateSizeOnDeallocate,
    size_t stSizeCurrent,
    const char* pcFile,
    int32_t iLine,
    const char* pcFunction
) {
    if (eEventType != NI_ALIGNEDREALLOC) {
        return MemoryManager::Instance().Reallocate(pvMemory, stSizeInBytes, pcFile, iLine, pcFunction);
    }
    uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
    if (!pvMemory) {
        return nullptr;
    }
    uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
    char* pcBlock = static_cast<char*>(
        MemoryManager::Instance().Reallocate(pucMemory - pucMemory[-1], stSizeInBytes + ucAlignment)
    );
    // The block is untyped heap memory: the padding comes from its address's low byte.
    uint8_t ucOffset = ucAlignment -
        (static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pcBlock)) & static_cast<uint8_t>(ucAlignment - 1));
    char* pmem = pcBlock + ucOffset;
    pmem[-1] = static_cast<char>(ucOffset);
    return pmem;
}

void* BSNiAllocator::AllocateExternal(
    size_t& stSizeInBytes,
    size_t& stAlignment,
    NiMemEventType eEventType,
    bool bProvideAccurateSizeOnDeallocate,
    const char* pcFile,
    int32_t iLine,
    const char* pcFunction
) {
    char* pmem;
    if (eEventType == NI_EXTERNAL_ALIGNEDMALLOC) {
        uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
        char* pcBlock = static_cast<char*>(MemoryManager::Instance().Allocate(stSizeInBytes + ucAlignment));
        // The block is untyped heap memory: the padding comes from its address's low byte.
        uint8_t ucOffset = ucAlignment -
            (static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pcBlock)) & static_cast<uint8_t>(ucAlignment - 1));
        pmem = pcBlock + ucOffset;
        pmem[-1] = static_cast<char>(ucOffset);
    }
    else {
        pmem = static_cast<char*>(MemoryManager::Instance().Allocate(stSizeInBytes, pcFile, iLine, pcFunction));
    }
    std::memset(pmem, 0, stSizeInBytes);
    return pmem;
}

void BSNiAllocator::DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) {
    if (eEventType == NI_EXTERNAL_ALIGNEDFREE) {
        if (pvMemory) {
            MemoryManager::Instance().Deallocate(
                static_cast<char*>(pvMemory) - static_cast<uint8_t>(*(static_cast<char*>(pvMemory) - 1))
            );
        }
    }
    else {
        MemoryManager::Instance().Deallocate(pvMemory);
    }
}

void* BSNiAllocator::ReallocateExternal(
    void* pvMemory,
    size_t& stSizeInBytes,
    size_t& stAlignment,
    NiMemEventType eEventType,
    bool bProvideAccurateSizeOnDeallocate,
    size_t stSizeCurrent,
    const char* pcFile,
    int32_t iLine,
    const char* pcFunction
) {
    if (eEventType != NI_EXTERNAL_ALGINEDREALLOC) {
        return MemoryManager::Instance().Reallocate(pvMemory, stSizeInBytes, pcFile, iLine, pcFunction);
    }
    uint8_t ucAlignment = static_cast<uint8_t>(stAlignment);
    if (!pvMemory) {
        return nullptr;
    }
    uint8_t* pucMemory = static_cast<uint8_t*>(pvMemory);
    char* pcBlock = static_cast<char*>(
        MemoryManager::Instance().Reallocate(pucMemory - pucMemory[-1], stSizeInBytes + ucAlignment)
    );
    // The block is untyped heap memory: the padding comes from its address's low byte.
    uint8_t ucOffset = ucAlignment -
        (static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pcBlock)) & static_cast<uint8_t>(ucAlignment - 1));
    char* pmem = pcBlock + ucOffset;
    pmem[-1] = static_cast<char>(ucOffset);
    return pmem;
}

// Every address passes.
bool BSNiAllocator::VerifyAddress(const void* pvMemory) {
    return true;
}
