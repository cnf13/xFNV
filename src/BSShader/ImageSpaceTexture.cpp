#include "BSShader/ImageSpaceTexture.h"

#include "BSShader/BSRenderedTexture.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

ImageSpaceTexture& ImageSpaceTexture::operator=(ImageSpaceTexture& aTex) {
    bIsRenderedTexture = aTex.bIsRenderedTexture;
    bIsTextureBorrowed = aTex.bIsTextureBorrowed;
    spTexture = aTex.spTexture;
    eFilterMode = aTex.eFilterMode;
    aTex.bIsTextureBorrowed = false;
    return *this;
}

void ImageSpaceTexture::SetBorrowed(bool abBorrowed) {
    bIsTextureBorrowed = abBorrowed;
}

NiTexture* ImageSpaceTexture::GetTexture() const {
    if (bIsRenderedTexture) {
        return static_cast<BSRenderedTexture*>(spTexture.m_pObject)->GetTexture(0);
    }

    return static_cast<NiTexture*>(spTexture.m_pObject);
}

void ImageSpaceTexture::SetClampMode(NiTexturingProperty::ClampMode aiMode) {
    eClampMode = static_cast<uint32_t>(aiMode);
}

void ImageSpaceTexture::SetFilterMode(NiTexturingProperty::FilterMode aiMode) {
    eFilterMode = static_cast<uint32_t>(aiMode);
}
