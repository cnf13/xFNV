#pragma once

#include "BSShader/BSTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiD3DTextureStage;

// A texture slot of an image space effect.
struct ImageSpaceTexture {
    ImageSpaceTexture()
      : bForceAniso(false), bIsRenderedTexture(false), bIsTextureBorrowed(false), spTexture(nullptr), eFilterMode(0),
        eClampMode(0) {}
    ~ImageSpaceTexture();

    void ReturnRenderedTexture();
    // Empties the slot.
    void ClearTexture();
    void BorrowRenderedTexture(BSTextureManager::eTextureType aeType);
    void SetTexture(NiTexture* apRendTex);
    // Holds a rendered texture in the slot and marks it as one.
    void SetTexture(BSRenderedTexture* apRendTex);
    // User-declared copy assignment: copies bIsRenderedTexture and bIsTextureBorrowed, takes aTex's texture, copies
    // eFilterMode and clears aTex's bIsTextureBorrowed, which is why aTex is not const. bForceAniso and eClampMode are
    // left as they are.
    ImageSpaceTexture& operator=(ImageSpaceTexture& aTex);
    BSRenderedTexture* GetRenderedTexture();
    void SetBorrowed(bool abBorrowed);
    NiTexture* GetTexture() const;
    void SetClampMode(NiTexturingProperty::ClampMode aiMode);
    void SetFilterMode(NiTexturingProperty::FilterMode aiMode);
    void SetupTexture(NiD3DTextureStage* apStage);

    // The image-space effects free their slots with MemoryManager::Deallocate.
    static void operator delete(void* apMemory);

    bool bForceAniso;
    bool bIsRenderedTexture;
    bool bIsTextureBorrowed;
    // An NiTexture, or a BSRenderedTexture when bIsRenderedTexture is set.
    NiPointer<NiObject> spTexture;
    uint32_t eFilterMode;
    uint32_t eClampMode;
};
