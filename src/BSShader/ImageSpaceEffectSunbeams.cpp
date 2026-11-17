#include "BSShader/ImageSpaceEffectSunbeams.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceShaderStencilCopy.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

NiPoint2 ImageSpaceEffectSunbeams::kSunPos(NiPoint2::ZERO);

ImageSpaceEffectSunbeams::ImageSpaceEffectSunbeams() : SwapTex() {}

// The shader parameters in slots 1 and 0 are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectSunbeams::~ImageSpaceEffectSunbeams() {
    for (int32_t i = 1; i >= 0; --i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(static_cast<uint32_t>(i), nullptr);
        }
    }
}

void ImageSpaceEffectSunbeams::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam2, std::nullopt, std::nullopt);
    ImageSpaceShaderParam* pparam;
    ImageSpaceShaderParam* pparam_2;
    SetEffectCount(2);
    pTextures.SetSize(3u);
    ImageSpaceShaderParam* pMemory =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam.m_pObject = pMemory;
    if (pMemory) {
        pparam = ::new (pMemory) ImageSpaceShaderParam;
    }
    else {
        pparam = nullptr;
    }
    pparam->InitConstantMap(0, 1);
    spParam.m_pObject = pparam;
    pEffectParamList.SetAt(0, spParam);
    SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[39], pparam, 2);
    pMemory = static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam2.m_pObject = pMemory;
    if (pMemory) {
        pparam_2 = ::new (pMemory) ImageSpaceShaderParam;
    }
    else {
        pparam_2 = nullptr;
    }
    pparam_2->InitConstantMap(0, 1);
    spParam2.m_pObject = pparam_2;
    pEffectParamList.SetAt(1u, spParam2);
    SetEffectInputs(1, 2, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(1, apManager->pEffectList.m_pBase[57], pparam_2, 0);
    pparam_2->SetPixelConstant(0, ImageSpaceEffectSunbeams::kSunPos.x, ImageSpaceEffectSunbeams::kSunPos.y, 0.0f, 0.0f);
}

void ImageSpaceEffectSunbeams::BorrowTextures(ImageSpaceEffectParam* apParam) {
    ImageSpaceTexture* p_SwapTex = &SwapTex;
    SwapTex.BorrowRenderedTexture(BSTextureManager::TEX_IMAGESPACE_SWAP);
    ImageSpaceTexture* a3 = p_SwapTex;
    pTextures.SetAt(2u, a3);
}

void ImageSpaceEffectSunbeams::ReturnTextures() {
    (&SwapTex)->ReturnRenderedTexture();
}

bool ImageSpaceEffectSunbeams::IsActive() {
    return (bool)((uint8_t)ImageSpaceManager::bUseSunbeams);
}

bool ImageSpaceEffectSunbeams::UpdateParams(ImageSpaceEffectParam* apParam) {
    pEffectParamList.m_pBase[1].m_pObject->SetPixelConstant(
        0, ImageSpaceEffectSunbeams::kSunPos.x, ImageSpaceEffectSunbeams::kSunPos.y, 0.0, 0.0
    );
    ImageSpaceShaderStencilCopy::uiStencilRef = 128;
    return false;
}
