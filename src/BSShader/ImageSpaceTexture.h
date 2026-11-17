#pragma once

#include "BSShader/BSTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiD3DTextureStage;

// A texture slot of an image space effect.
struct ImageSpaceTexture {
    ImageSpaceTexture();
    ~ImageSpaceTexture();

    void ReturnRenderedTexture();
    // Empties the slot.
    void ClearTexture();
    void BorrowRenderedTexture(BSTextureManager::eTextureType aeType);
    void SetTexture(NiTexture* apRendTex);
    // Holds a rendered texture in the slot and marks it as one.
    void SetTexture(BSRenderedTexture* apRendTex);
    // User-declared copy assignment: copies bIsRenderedTexture and bIsTextureBorrowed, takes aTex's texture (released
    // and referenced through the Interlocked functions), copies eFilterMode and clears aTex's bIsTextureBorrowed, which
    // is why aTex is not const. bForceAniso and eClampMode are left as they are.
    ImageSpaceTexture& operator=(ImageSpaceTexture& aTex);
    BSRenderedTexture* GetRenderedTexture();
    void SetBorrowed(bool abBorrowed);
    NiTexture* GetTexture() const;
    void SetClampMode(NiTexturingProperty::ClampMode aiMode);
    void SetFilterMode(NiTexturingProperty::FilterMode aiMode);
    void SetupTexture(NiD3DTextureStage* a2);

    // The image-space effects free their slots with MemoryManager::Deallocate.
    static void operator delete(void* apMemory);

    bool bForceAniso;
    bool bIsRenderedTexture;
    bool bIsTextureBorrowed;
    uint8_t padding03;
    void* spTexture;
    uint32_t eFilterMode;
    uint32_t eClampMode;
};
