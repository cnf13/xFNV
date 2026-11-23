#include "BSShader/ImageSpaceEffectNoise.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// Registers the instance, copies the default permutation table into both halves of p and clears the shared scroll,
// scale and amplitude colours (their alpha is left as it is).
ImageSpaceEffectNoise::ImageSpaceEffectNoise()
  : pNoiseMap(), pTexBuffer(), pPermutedTex2D(), pPermutedGradientTex(), bIsLoaded(false) {
    pInstance = this;
    for (uint32_t i = 0; i < 256; ++i) {
        p[i + 256] = permutation[i];
        p[i] = permutation[i];
    }
    for (NiColorA* pColor : {&fTexScroll0, &fTexScroll1, &fTexScroll2, &fTexScale, &fAmplitude}) {
        pColor->r = 0.0f;
        pColor->g = 0.0f;
        pColor->b = 0.0f;
    }
}

// The two shader parameters are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectNoise::~ImageSpaceEffectNoise() {
    for (uint32_t i = 0; i < 2; ++i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
}

void ImageSpaceEffectNoise::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceEffect* peffect;
    ImageSpaceEffect* peffect_2;
    if (bParamsChanged) {
        bParamsChanged = UpdateParams(apParam);
    }
    pTexBuffer.BorrowRenderedTexture(BSTextureManager::TEX_NOISE_BUFFER);
    ImageSpaceEffect* m_pObject = pEffectList.m_pBase->m_pObject;
    if (!m_pObject || m_pObject == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
        peffect = nullptr;
    }
    else {
        peffect = pEffectList.m_pBase->m_pObject;
    }
    ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase->m_pObject;
    pparam->SetPixelConstant(0, ImageSpaceEffectNoise::fTexScroll0.r, ImageSpaceEffectNoise::fTexScroll0.g, 0.0f, 0.0f);
    pparam->SetPixelConstant(1, ImageSpaceEffectNoise::fTexScroll1.r, ImageSpaceEffectNoise::fTexScroll1.g, 0.0f, 0.0f);
    pparam->SetPixelConstant(2, ImageSpaceEffectNoise::fTexScroll2.r, ImageSpaceEffectNoise::fTexScroll2.g, 0.0f, 0.0f);
    pparam->SetPixelConstant(3, ImageSpaceEffectNoise::fNoiseScale, 0.0f, 0.0f, 0.0f);
    pparam->SetPixelConstant(
        4,
        ImageSpaceEffectNoise::fTexScale.r,
        ImageSpaceEffectNoise::fTexScale.g,
        ImageSpaceEffectNoise::fTexScale.b,
        0.0f
    );
    pparam->SetPixelConstant(
        5,
        ImageSpaceEffectNoise::fAmplitude.r,
        ImageSpaceEffectNoise::fAmplitude.g,
        ImageSpaceEffectNoise::fAmplitude.b,
        0.0f
    );
    peffect->SetTexture(0, &pTexBuffer, NiTexturingProperty::FILTER_NEAREST);
    peffect->SetTexture(1, pTextures.m_pBase[1], NiTexturingProperty::FILTER_BILERP);
    (*pTextures.m_pBase)->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    pTextures.m_pBase[1]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    peffect->Render(apGeometry, apRenderer, pparam, 1);
    ImageSpaceEffect* v9 = pEffectList.m_pBase[1].m_pObject;
    if (!v9 || v9 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
        peffect_2 = nullptr;
    }
    else {
        peffect_2 = pEffectList.m_pBase[1].m_pObject;
    }
    ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase[1].m_pObject;
    pparam_2
        ->SetPixelConstant(0, ImageSpaceEffectNoise::fTexScroll0.r, ImageSpaceEffectNoise::fTexScroll0.g, 0.0f, 0.0f);
    pparam_2
        ->SetPixelConstant(1, ImageSpaceEffectNoise::fTexScroll1.r, ImageSpaceEffectNoise::fTexScroll1.g, 0.0f, 0.0f);
    pparam_2
        ->SetPixelConstant(2, ImageSpaceEffectNoise::fTexScroll2.r, ImageSpaceEffectNoise::fTexScroll2.g, 0.0f, 0.0f);
    pparam_2->SetPixelConstant(3, ImageSpaceEffectNoise::fNoiseScale, 0.0f, 0.0f, 0.0f);
    peffect_2->SetTexture(0, *pTextures.m_pBase, NiTexturingProperty::FILTER_NEAREST);
    peffect_2->SetTexture(1, &pTexBuffer, NiTexturingProperty::FILTER_NEAREST);
    (*pTextures.m_pBase)->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    pTextures.m_pBase[1]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    peffect_2->Render(apGeometry, apRenderer, pparam_2, 1);
    pTexBuffer.ReturnRenderedTexture();
}

// Two effects, each with new shader parameters (3 vertex and 6 pixel constants, no offset on texture 1): the manager's
// effects 114 and 115. Seeds the tables first like UpdateParams.
void ImageSpaceEffectNoise::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(2);
    pTextures.SetSize(4);
    if (!bIsLoaded) {
        InitializeUtilityBuffers(static_cast<uint32_t>(static_cast<int64_t>(BSShaderManager::fTimerValues[1])));
        bIsLoaded = true;
    }
    ImageSpaceShaderParam* pParam = new ImageSpaceShaderParam;
    pParam->InitConstantMap(3, 6);
    pParam->bUseTexOffset[1] = false;
    pEffectParamList.SetAt(0, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    SetEffect(0, apManager->pEffectList.m_pBase[114], pParam, 0);
    pParam = new ImageSpaceShaderParam;
    pParam->InitConstantMap(3, 6);
    pParam->bUseTexOffset[1] = false;
    pEffectParamList.SetAt(1, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    SetEffect(1, apManager->pEffectList.m_pBase[115], pParam, 0);
}

bool ImageSpaceEffectNoise::IsActive() {
    return true;
}

// Seeds the tables once from the second shader timer. The float is converted to a 64-bit integer and its low 32 bits
// are passed.
bool ImageSpaceEffectNoise::UpdateParams(ImageSpaceEffectParam* pParam) {
    if (bIsLoaded) {
        return true;
    }
    InitializeUtilityBuffers(static_cast<uint32_t>(static_cast<int64_t>(BSShaderManager::fTimerValues[1])));
    bIsLoaded = true;
    return true;
}

bool ImageSpaceEffectNoise::IsLoaded() {
    return bIsLoaded;
}

void ImageSpaceEffectNoise::InitializeUtilityBuffers(uint32_t aiSeed) {
    uint32_t i;
    int8_t v3;
    for (i = 0; i < 256; ++i) {
        v3 = permutation[static_cast<uint8_t>(i + ((1402024253 * aiSeed + 586950981) >> 24))];
        p[i + 256] = v3;
        p[i] = v3;
    }
}

// Seeds the tables from the second shader timer, as UpdateParams does, and marks them loaded. The float is converted
// through a 64-bit integer and its low 32 bits are passed.
void ImageSpaceEffectNoise::ShufflePermuationTable() {
    InitializeUtilityBuffers(static_cast<uint32_t>(static_cast<int64_t>(BSShaderManager::fTimerValues[1])));
    bIsLoaded = true;
}

ImageSpaceEffectNoise* ImageSpaceEffectNoise::GetInstance() {
    return ImageSpaceEffectNoise::pInstance;
}
