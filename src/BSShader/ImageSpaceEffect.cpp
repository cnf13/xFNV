#include "BSShader/ImageSpaceEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

void ImageSpaceEffect_OutputArray_SetAt(
    NiTPrimitiveArray<int32_t>* apArray,
    uint32_t auiIndex,
    const int32_t& aiElement
) {
    apArray->SetAt(auiIndex, aiElement);
}

void ImageSpaceEffect_OutputArray_SetSize(NiTPrimitiveArray<int32_t>* apArray, uint32_t auiMaxSize) {
    apArray->SetSize(auiMaxSize);
}

ImageSpaceEffect::ImageSpaceEffect()
  : bIsActive(false), bParamsChanged(true), pEffectList(0, 1), pEffectParamList(0, 1), pTextures(0, 1),
    pEffectInputs(0, 1), pEffectOutput(0, 1) {}

// The arrays free their own buffers; the inputs they point to are the effect's.
ImageSpaceEffect::~ImageSpaceEffect() {
    for (uint32_t i = 0; i < pEffectInputs.m_usSize; ++i) {
        if (pEffectInputs.m_pBase[i]) {
            MemoryManager_Singleton.Deallocate(pEffectInputs.m_pBase[i]);
            pEffectInputs.SetAt(i, nullptr);
        }
    }
}

void ImageSpaceEffect::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceEffect* peffect;
    BorrowTextures(apParam);
    for (uint32_t i = 0; i < pEffectList.m_usSize;
         peffect->Render(apGeometry, apRenderer, pEffectParamList.m_pBase[i++].m_pObject, abEndFrame)) {
        ApplyEffectTextures(i);
        UpdateParams(apParam);
        peffect = pEffectList.m_pBase[i].m_pObject;
    }
    ReturnTextures();
}

bool ImageSpaceEffect::IsActive() {
    return (bool)((uint8_t)bIsActive);
}

bool ImageSpaceEffect::UpdateParams(ImageSpaceEffectParam* pParam) {
    return false;
}

// Each array's SetAt stores the pointer with no reference counting: the pointees are not NiRefObjects.
void ImageSpaceEffect::SetEffect(
    int32_t aiEffectIndex,
    ImageSpaceEffect* apEffect,
    ImageSpaceEffectParam* apParam,
    int32_t aiOutput
) {
    pEffectList.SetAt(static_cast<uint32_t>(aiEffectIndex), NiPointer<ImageSpaceEffect>(apEffect));
    pEffectParamList.SetAt(
        static_cast<uint32_t>(aiEffectIndex),
        NiPointer<ImageSpaceShaderParam>(static_cast<ImageSpaceShaderParam*>(apParam))
    );
    ImageSpaceEffect_OutputArray_SetAt(&pEffectOutput, static_cast<uint32_t>(aiEffectIndex), aiOutput);
}

// The textures array keeps its size.
void ImageSpaceEffect::SetEffectCount(int32_t aiCount) {
    pEffectList.SetSize(aiCount);
    pEffectParamList.SetSize(aiCount);
    pEffectInputs.SetSize(aiCount);
    ImageSpaceEffect_OutputArray_SetSize(&pEffectOutput, aiCount);
}

// Builds the two inputs on the heap and sets them at the effect index.
void ImageSpaceEffect::SetEffectInputs(
    int32_t auiEffectIndex,
    int32_t auiInput1,
    int32_t auiInput2,
    NiTexturingProperty::FilterMode aeFilterMode1,
    NiTexturingProperty::FilterMode aeFilterMode2
) {
    ImageSpaceEffect::EffectInput* piinputtex = static_cast<ImageSpaceEffect::EffectInput*>(
        MemoryManager_Singleton.Allocate(2 * sizeof(ImageSpaceEffect::EffectInput))
    );
    piinputtex->iTexIndex = auiInput1;
    piinputtex->iFilterMode = aeFilterMode1;
    piinputtex[1].iTexIndex = auiInput2;
    piinputtex[1].iFilterMode = aeFilterMode2;
    pEffectInputs.SetAt(static_cast<uint32_t>(auiEffectIndex), piinputtex);
}

// Builds the one input on the heap and sets it at the effect index; the index goes to SetAt unchanged.
void ImageSpaceEffect::SetEffectInputs(
    int32_t auiEffectIndex,
    int32_t auiInput,
    NiTexturingProperty::FilterMode aeFilterMode
) {
    ImageSpaceEffect::EffectInput* piinputtex = static_cast<ImageSpaceEffect::EffectInput*>(
        MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffect::EffectInput))
    );
    piinputtex->iTexIndex = auiInput;
    piinputtex->iFilterMode = aeFilterMode;
    pEffectInputs.SetAt(static_cast<uint32_t>(auiEffectIndex), piinputtex);
}

// Builds the four inputs on the heap and sets them at the effect index.
void ImageSpaceEffect::SetEffectInputs(
    int32_t auiEffectIndex,
    int32_t auiInput1,
    int32_t auiInput2,
    int32_t auiInput3,
    int32_t auiInput4,
    NiTexturingProperty::FilterMode aeFilterMode1,
    NiTexturingProperty::FilterMode aeFilterMode2,
    NiTexturingProperty::FilterMode aeFilterMode3,
    NiTexturingProperty::FilterMode aeFilterMode4
) {
    ImageSpaceEffect::EffectInput* piinputtex = static_cast<ImageSpaceEffect::EffectInput*>(
        MemoryManager_Singleton.Allocate(4 * sizeof(ImageSpaceEffect::EffectInput))
    );
    piinputtex->iTexIndex = auiInput1;
    piinputtex->iFilterMode = aeFilterMode1;
    piinputtex[1].iTexIndex = auiInput2;
    piinputtex[1].iFilterMode = aeFilterMode2;
    piinputtex[2].iTexIndex = auiInput3;
    piinputtex[2].iFilterMode = aeFilterMode3;
    piinputtex[3].iTexIndex = auiInput4;
    piinputtex[3].iFilterMode = aeFilterMode4;
    pEffectInputs.SetAt(static_cast<uint32_t>(auiEffectIndex), piinputtex);
}

void ImageSpaceEffect::SetTexture(
    int32_t aiIndex,
    ImageSpaceTexture* apTex,
    NiTexturingProperty::FilterMode aiFilterMode
) {
    pTextures.SetAt(aiIndex, apTex);
    if (apTex) {
        apTex->SetFilterMode(aiFilterMode);
    }
}

// Binds the effect's inputs to the sub-effect's texture slots from 1 on (the shared SetAt, then the texture's filter
// mode), then its output texture to slot 0 with the nearest filter.
void ImageSpaceEffect::ApplyEffectTextures(int32_t aiEffectIndex) {
    ImageSpaceEffect* pEffect = pEffectList.m_pBase[aiEffectIndex].m_pObject;
    int32_t iMaxSize = pEffect->pTextures.m_usMaxSize;
    ImageSpaceEffect::EffectInput* pInput = pEffectInputs.m_pBase[aiEffectIndex];
    int32_t iOutput = pEffectOutput.m_pBase[aiEffectIndex];
    for (int32_t i = 1; i < iMaxSize; ++i, ++pInput) {
        if (pInput->iTexIndex >= 0) {
            ImageSpaceTexture* pTexture = pTextures.m_pBase[pInput->iTexIndex];
            NiTexturingProperty::FilterMode eFilterMode = pInput->iFilterMode;
            pEffect->pTextures.SetAt(static_cast<uint32_t>(i), pTexture);
            if (pTexture) {
                pTexture->SetFilterMode(eFilterMode);
            }
        }
    }
    ImageSpaceTexture* pOutput = pTextures.m_pBase[iOutput];
    // EXE-INLINED 0xBA3AA0
    if (pEffect->pTextures.m_usSize > 0) {
        if (pOutput) {
            if (!pEffect->pTextures.m_pBase[0]) {
                ++pEffect->pTextures.m_usESize;
            }
        }
        else {
            if (pEffect->pTextures.m_pBase[0]) {
                --pEffect->pTextures.m_usESize;
            }
        }
    }
    else {
        pEffect->pTextures.m_usSize = 1;
        if (pOutput) {
            ++pEffect->pTextures.m_usESize;
        }
    }
    pEffect->pTextures.m_pBase[0] = pOutput;
    if (pOutput) {
        pOutput->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    }
}

// Builds the three inputs on the heap and sets them at the effect index.
void ImageSpaceEffect::SetEffectInputs(
    int32_t aiEffectIndex,
    int32_t aiInput1,
    int32_t aiInput2,
    int32_t aiInput3,
    NiTexturingProperty::FilterMode aiFilterMode1,
    NiTexturingProperty::FilterMode aiFilterMode2,
    NiTexturingProperty::FilterMode aiFilterMode3
) {
    ImageSpaceEffect::EffectInput* piinputtex = static_cast<ImageSpaceEffect::EffectInput*>(
        MemoryManager_Singleton.Allocate(3 * sizeof(ImageSpaceEffect::EffectInput))
    );
    piinputtex->iTexIndex = aiInput1;
    piinputtex->iFilterMode = aiFilterMode1;
    piinputtex[1].iTexIndex = aiInput2;
    piinputtex[1].iFilterMode = aiFilterMode2;
    piinputtex[2].iTexIndex = aiInput3;
    piinputtex[2].iFilterMode = aiFilterMode3;
    pEffectInputs.SetAt(static_cast<uint32_t>(aiEffectIndex), piinputtex);
}

void ImageSpaceEffect::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// The base's Render, but each sub-effect runs only when its switch in pEffectOn is on. ImageSpaceEffectDepthOfField
// inherits it.
void ImageSpaceEffectOption::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    BorrowTextures(apParam);
    for (uint32_t i = 0; i < pEffectList.m_usSize; ++i) {
        if (pEffectOn.m_pBase[i]) {
            ApplyEffectTextures(i);
            UpdateParams(apParam);
            ImageSpaceEffect* pEffect = pEffectList.m_pBase[i].m_pObject;
            pEffect->Render(apGeometry, apRenderer, pEffectParamList.m_pBase[i].m_pObject, abEndFrame);
        }
    }
    ReturnTextures();
}
