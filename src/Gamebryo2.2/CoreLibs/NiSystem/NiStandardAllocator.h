#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"

class NiSmallObjectAllocator;

// Gamebryo's standard allocator: small requests go to the small object allocator, the rest to the game's MemoryManager.
// It has no fields beyond the vtable pointer.
class NiStandardAllocator : public NiAllocator {
public:
    void* Allocate(
        size_t& stSizeInBytes,
        size_t& stAlignment,
        NiMemEventType eEventType,
        int32_t bProvideAccurateSizeOnDeallocate,
        const char* pcFile,
        int32_t iLine,
        const char* pcFunction
    ) override;
    void Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) override;
    void* Reallocate(
        void* pvMemory,
        size_t& stSizeInBytes,
        size_t& stAlignment,
        NiMemEventType eEventType,
        int32_t bProvideAccurateSizeOnDeallocate,
        size_t stSizeCurrent,
        const char* pcFile,
        int32_t iLine,
        const char* pcFunction
    ) override;
    void* AllocateExternal(
        size_t& stSizeInBytes,
        size_t& stAlignment,
        NiMemEventType eEventType,
        bool bProvideAccurateSizeOnDeallocate,
        const char* pcFile,
        int32_t iLine,
        const char* pcFunction
    ) override;
    void DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) override;
    void* ReallocateExternal(
        void* pvMemory,
        size_t& stSizeInBytes,
        size_t& stAlignment,
        NiMemEventType eEventType,
        bool bProvideAccurateSizeOnDeallocate,
        size_t stSizeCurrent,
        const char* pcFile,
        int32_t iLine,
        const char* pcFunction
    ) override;
    void Initialize() override;
    void Shutdown() override;
    bool VerifyAddress(const void* pvMemory) override;

    // The size of the block at apMemory, kept in its first four bytes (unaligned, read byte by byte).
    static uint32_t GetSizeFromAddress(void* pMemory);

    static inline NiSmallObjectAllocator* ms_pkSmallAlloc;
};
