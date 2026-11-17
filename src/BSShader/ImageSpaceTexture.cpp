#include "BSShader/ImageSpaceTexture.h"

#include "BSShader/BSRenderedTexture.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

ImageSpaceTexture::ImageSpaceTexture()
  : bForceAniso(false), bIsRenderedTexture(false), bIsTextureBorrowed(false), spTexture(nullptr), eFilterMode(0),
    eClampMode(0) {}

// The slot holds an NiTexture or a BSRenderedTexture, both NiRefObjects.
ImageSpaceTexture& ImageSpaceTexture::operator=(ImageSpaceTexture& aTex) {
    bIsRenderedTexture = aTex.bIsRenderedTexture;
    bIsTextureBorrowed = aTex.bIsTextureBorrowed;
    NiRefObject* pOld = static_cast<NiRefObject*>(spTexture);
    if (pOld != aTex.spTexture) {
        if (pOld) {
            pOld->DecRefCount();
        }
        NiRefObject* pNew = static_cast<NiRefObject*>(aTex.spTexture);
        spTexture = pNew;
        if (pNew) {
            pNew->IncRefCount();
        }
    }
    eFilterMode = aTex.eFilterMode;
    aTex.bIsTextureBorrowed = false;
    return *this;
}

void ImageSpaceTexture::SetBorrowed(bool abBorrowed) {
    bIsTextureBorrowed = (bool)((uint8_t)abBorrowed);
}

NiTexture* ImageSpaceTexture::GetTexture() const {
    if (bIsRenderedTexture) {
        return static_cast<BSRenderedTexture*>(spTexture)->GetTexture(0);
    }

    return static_cast<NiTexture*>(spTexture);
}

void ImageSpaceTexture::SetClampMode(NiTexturingProperty::ClampMode aiMode) {
    eClampMode = (uint32_t)((int32_t)aiMode);
}

void ImageSpaceTexture::SetFilterMode(NiTexturingProperty::FilterMode aiMode) {
    eFilterMode = static_cast<uint32_t>(aiMode);
}
