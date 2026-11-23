#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"

// The allocator Gamebryo's NiMalloc/NiFree and NiMemObject new/delete go through: it forwards to the game's
// MemoryManager. It has no fields beyond the vtable pointer.
class BSNiAllocator : public NiAllocator {
public:
    BSNiAllocator();

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
    void Initialize() override {}
    void Shutdown() override {}
    bool VerifyAddress(const void* pvMemory) override;
};
