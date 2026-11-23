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
    ImageSpaceShaderParam* pparam;
    ImageSpaceShaderParam* pparam_2;
    SetEffectCount(2);
    pTextures.SetSize(3u);
    pparam = new ImageSpaceShaderParam;
    pparam->InitConstantMap(0, 1);
    pEffectParamList.SetAt(0, NiPointer<ImageSpaceShaderParam>(pparam));
    SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[39], pparam, 2);
    pparam_2 = new ImageSpaceShaderParam;
    pparam_2->InitConstantMap(0, 1);
    pEffectParamList.SetAt(1u, NiPointer<ImageSpaceShaderParam>(pparam_2));
    SetEffectInputs(1, 2, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(1, apManager->pEffectList.m_pBase[57], pparam_2, 0);
    pparam_2->SetPixelConstant(0, ImageSpaceEffectSunbeams::kSunPos.x, ImageSpaceEffectSunbeams::kSunPos.y, 0.0f, 0.0f);
}

void ImageSpaceEffectSunbeams::BorrowTextures(ImageSpaceEffectParam* apParam) {
    ImageSpaceTexture* p_SwapTex = &SwapTex;
    SwapTex.BorrowRenderedTexture(BSTextureManager::TEX_IMAGESPACE_SWAP);
    ImageSpaceTexture* pSwapTexture = p_SwapTex;
    pTextures.SetAt(2u, pSwapTexture);
}

void ImageSpaceEffectSunbeams::ReturnTextures() {
    SwapTex.ReturnRenderedTexture();
}

bool ImageSpaceEffectSunbeams::IsActive() {
    return ImageSpaceManager::bUseSunbeams;
}

bool ImageSpaceEffectSunbeams::UpdateParams(ImageSpaceEffectParam* apParam) {
    pEffectParamList.m_pBase[1]
        ->SetPixelConstant(0, ImageSpaceEffectSunbeams::kSunPos.x, ImageSpaceEffectSunbeams::kSunPos.y, 0.0, 0.0);
    ImageSpaceShaderStencilCopy::uiStencilRef = 128;
    return false;
}
