#include "BSShader/ImageSpaceEffectGetHit.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "SDK/MSVC/ehvec.h"

ImageSpaceEffectGetHit::ImageSpaceEffectGetHit() {
    pTextures.SetSize(4u);
}

// The two blur textures in slots 2 and 3 and the shader parameters in slots 1 and 3 are the effect's.
ImageSpaceEffectGetHit::~ImageSpaceEffectGetHit() {
    for (uint32_t i = 2; i < 4; ++i) {
        if (ImageSpaceTexture* pTexture = pTextures.m_pBase[i]) {
            pTexture->~ImageSpaceTexture();
            MemoryManager_Singleton.Deallocate(pTexture);
            pTextures.SetAt(i, nullptr);
        }
    }
    for (uint32_t i = 1; i < 4; i += 2) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
}

void ImageSpaceEffectGetHit::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(4);
    pTextures.SetSize(4u);
    ImageSpaceShaderParam* pparam = nullptr;
    pEffectParamList.SetAt(0, nullptr);
    SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[34], nullptr, 2);
    pEffectParamList.SetAt(1u, nullptr);
    SetEffectInputs(1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(1, apManager->pEffectList.m_pBase[17], nullptr, 2);
    pEffectParamList.SetAt(2u, nullptr);
    SetEffectInputs(2, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(2, apManager->pEffectList.m_pBase[17], nullptr, 2);
    ImageSpaceShaderParam* v6 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    if (v6) {
        pparam = ::new (v6) ImageSpaceShaderParam;
    }
    pparam->InitConstantMap(0, 2);
    pparam->bUseTexOffset[2] = false;
    pEffectParamList.SetAt(3u, pparam);
    SetEffectInputs(3, 1, 2, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(3, apManager->pEffectList.m_pBase[44], pparam, 0);
}

void ImageSpaceEffectGetHit::BorrowTextures(ImageSpaceEffectParam* pParam) {
    ImageSpaceTexture* ptex = pBufferA;
    pBufferA->BorrowRenderedTexture(BSTextureManager::TEX_GETHIT_BUFFER);
    pTextures.SetAt(2u, ptex);
    ptex = &pBufferA[1];
    pBufferA[1].BorrowRenderedTexture(BSTextureManager::TEX_GETHIT_BUFFER);
    pTextures.SetAt(3u, ptex);
}

// Hands back the two blur textures and clears their slots.
void ImageSpaceEffectGetHit::ReturnTextures() {
    pTextures.m_pBase[TEXTURE_BLUR1]->ReturnRenderedTexture();
    pTextures.SetAt(TEXTURE_BLUR1, nullptr);
    pTextures.m_pBase[TEXTURE_BLUR2]->ReturnRenderedTexture();
    pTextures.SetAt(TEXTURE_BLUR2, nullptr);
}

bool ImageSpaceEffectGetHit::IsActive() {
    return ImageSpaceEffectGetHit::fAmplitude > 0.0 || ImageSpaceEffectGetHit::fTriggerValue > 0.0;
}

// Constant 0 holds the blur strength and the trigger value; constant 1 the double-vision offset, which circles
// once an hour of the timer.
bool ImageSpaceEffectGetHit::UpdateParams(ImageSpaceEffectParam* pParam) {
    ImageSpaceShaderParam* pShaderParam = pEffectParamList.m_pBase[EFFECT_DOUBLEVISION].m_pObject;
    float fAngle = (float)((double)BSShaderManager::fTimerValues[0] / 3600.0 * 6.283180236816406);
    float fTrigger = fTriggerValue;
    float fBlur = fTrigger * fBlurAmount;
    pShaderParam->SetPixelConstant(0, 0.0f, 0.0f, fBlur, fTrigger);
    float fCos = std::cos(fAngle);
    float fOffsetY = fCos * fAmplitude;
    float fSin = std::sin(fAngle);
    float fOffsetX = fSin * fAmplitude;
    pShaderParam->SetPixelConstant(1, fOffsetX, fOffsetY, 1.0f, 1.0f);
    ImageSpaceEffectBlur::fBlurRadius = 2.0f;
    return true;
}

// Marks the parameters changed and takes the data array.
void ImageSpaceEffectGetHit::SetParams(float* apfData) {
    bParamsChanged = true;
    pfData = apfData;
}
