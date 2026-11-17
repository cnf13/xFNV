#include "BSShader/ImageSpaceEffectInterface.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BoltShader.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

NiPointer<NiTexture> ImageSpaceEffectInterface::spScanlinesTexture;
NiPointer<NiTexture> ImageSpaceEffectInterface::spDistortTexture;
NiPointer<NiTexture> ImageSpaceEffectInterface::spMaskTexture;

ImageSpaceEffectInterface::ImageSpaceEffectInterface() {
    pTextures.SetSize(5u);
    bIsActive = false;
    kTint = NiColorA_WHITE;
}

void ImageSpaceEffectInterface::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceEffect* v7;
    BorrowTextures(apParam);
    ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase[ImageSpaceEffectInterface::eMode].m_pObject;
    ApplyEffectTextures(ImageSpaceEffectInterface::eMode);
    UpdateParams(m_pObject);
    if (ImageSpaceEffectInterface::eMode == 1) {
        v7 = pEffectList.m_pBase[1].m_pObject;
    }
    else {
        v7 = pEffectList.m_pBase->m_pObject;
    }
    v7->Render(apGeometry, apRenderer, m_pObject, abEndFrame);
    ReturnTextures();
}

void ImageSpaceEffectInterface::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    bIsActive = false;
    SetEffectCount(2);
    NiTPrimitiveArray<ImageSpaceTexture*>* p_pTextures = &pTextures;
    pTextures.SetSize(5u);
    BSShaderManager::GetTexture(
        "Data\\Textures\\Pipboy3000\\PipboyScanlines.dds", true, ImageSpaceEffectInterface::spScanlinesTexture, false,
        false
    );
    NiTexture* pScanlineTexture = ImageSpaceEffectInterface::spScanlinesTexture.m_pObject;
    BSShaderManager::GetTexture(
        "Data\\Textures\\Pipboy3000\\PipboyDistortEffectMap.dds", true, ImageSpaceEffectInterface::spDistortTexture,
        false, false
    );
    NiTexture* pDistortTexture = ImageSpaceEffectInterface::spDistortTexture.m_pObject;
    ImageSpaceTexture* v6 =
        static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
    if (v6) {
        v6->bForceAniso = false;
        v6->bIsRenderedTexture = false;
        v6->bIsTextureBorrowed = false;
        v6->spTexture = nullptr;
        v6->eFilterMode = 0;
        v6->eClampMode = 0;
    }
    else {
        v6 = nullptr;
    }
    p_pTextures->SetAt(3u, v6);
    pTextures.m_pBase[3]->SetTexture(pScanlineTexture);
    pTextures.m_pBase[3]->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
    ImageSpaceTexture* v7 =
        static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
    if (v7) {
        v7->bForceAniso = false;
        v7->bIsRenderedTexture = false;
        v7->bIsTextureBorrowed = false;
        v7->spTexture = nullptr;
        v7->eFilterMode = 0;
        v7->eClampMode = 0;
    }
    else {
        v7 = nullptr;
    }
    p_pTextures->SetAt(4u, v7);
    pTextures.m_pBase[4]->SetTexture(pDistortTexture);
    pTextures.m_pBase[4]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    ImageSpaceTexture* v8 =
        static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
    if (v8) {
        v8->bForceAniso = false;
        v8->bIsRenderedTexture = false;
        v8->bIsTextureBorrowed = false;
        v8->spTexture = nullptr;
        v8->eFilterMode = 0;
        v8->eClampMode = 0;
    }
    else {
        v8 = nullptr;
    }
    p_pTextures->SetAt(2u, v8);
    pTextures.m_pBase[2]->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
    pTextures.m_pBase[2]->SetTexture(static_cast<NiTexture*>(nullptr));
    ImageSpaceEffectInterface::spMaskTexture = nullptr;
    ImageSpaceTexture** m_pBase = pTextures.m_pBase;
    float foffsetx = 0.0f;
    float foffsety = 0.0f;
    // Each unsigned size is converted as a signed value plus the float 2^32 when negative, as the game does; that
    // addition rounds at the thread's precision control (24 bits under Direct3D), where a 64-bit conversion of the size
    // would not.
    if (*m_pBase) {
        NiTexture* ptex = (*m_pBase)->GetTexture();
        int32_t iWidth = static_cast<int32_t>(ptex->GetWidth());
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        foffsetx = 1.0 / dWidth * ImageSpaceEffectInterface::fBlurRadius;
        int32_t iHeight = static_cast<int32_t>(ptex->GetHeight());
        double dHeight = iHeight;
        if (iHeight < 0) {
            dHeight += 4294967296.0f;
        }
        foffsety = 1.0 / dHeight * ImageSpaceEffectInterface::fBlurRadius;
    }
    ImageSpaceShaderParam* pparam;
    ImageSpaceShaderParam* v12 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    if (v12) {
        pparam = ::new (v12) ImageSpaceShaderParam;
    }
    else {
        pparam = nullptr;
    }
    pparam->InitConstantMap(0, 4);
    pparam->bUseTexOffset[1] = true;
    pparam->bUseTexOffset[2] = true;
    pparam->SetPixelConstant(
        0, ImageSpaceEffectInterface::fBlurIntensity, ImageSpaceEffectInterface::fVerticalHold,
        ImageSpaceEffectInterface::fTextureUsedPart, ImageSpaceEffectInterface::fScanlineFrequency
    );
    float fDistortEverywhere = ImageSpaceEffectInterface::bDistortEverywhere ? 1.0f : 0.0f;
    pparam->SetPixelConstant(
        1, ImageSpaceEffectInterface::fDistortVerticalScale, ImageSpaceEffectInterface::fDistortVerticalPosition,
        ImageSpaceEffectInterface::fDistortHorizontalScale, fDistortEverywhere
    );
    pparam->SetPixelConstant(
        2, ImageSpaceEffectInterface::kTint.r, ImageSpaceEffectInterface::kTint.g, ImageSpaceEffectInterface::kTint.b,
        ImageSpaceEffectInterface::kTint.a
    );
    pparam->SetPixelConstant(3, foffsetx, foffsety, 0.0f, 0.0f);
    ImageSpaceShaderParam* pparam2;
    ImageSpaceShaderParam* v15 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    if (v15) {
        pparam2 = ::new (v15) ImageSpaceShaderParam;
    }
    else {
        pparam2 = nullptr;
    }
    pparam2->InitConstantMap(0, 5);
    pparam2->bUseTexOffset[1] = true;
    pparam2->bUseTexOffset[2] = true;
    pparam2->SetPixelConstant(
        0, ImageSpaceEffectInterface::fBlurIntensity, ImageSpaceEffectInterface::fVerticalHold,
        ImageSpaceEffectInterface::fTextureUsedPart, ImageSpaceEffectInterface::fScanlineFrequency
    );
    float fDistortEverywhere2 = ImageSpaceEffectInterface::bDistortEverywhere ? 1.0f : 0.0f;
    pparam2->SetPixelConstant(
        1, ImageSpaceEffectInterface::fDistortVerticalScale, ImageSpaceEffectInterface::fDistortVerticalPosition,
        ImageSpaceEffectInterface::fDistortHorizontalScale, fDistortEverywhere2
    );
    pparam2->SetPixelConstant(
        2, ImageSpaceEffectInterface::kTint.r, ImageSpaceEffectInterface::kTint.g, ImageSpaceEffectInterface::kTint.b,
        ImageSpaceEffectInterface::kTint.a
    );
    pparam2->SetPixelConstant(3, foffsetx, foffsety, 0.0f, 0.0f);
    pparam2->SetPixelConstant(4, 1.0f, 0.0f, 0.0f, 0.0f);
    // The parameter array holds raw pointers (its SetAt does no reference counting) and is passed the raw pointer's
    // address; the declared element type, NiPointer, does not model that.
    pEffectParamList.SetAt(0, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pparam));
    pEffectParamList.SetAt(1u, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pparam2));
    SetEffectInputs(
        0, 2, 1, 3, 4, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST,
        NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST
    );
    SetEffectInputs(
        1, 2, 1, 3, 4, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST,
        NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST
    );
    SetEffect(0, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_INTERFACE_SCANLINES_BLEND], pparam, 0);
    SetEffect(
        1, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_INTERFACE_VATS_SCANLINES_BLEND], pparam2, 0
    );
}

void ImageSpaceEffectInterface::ReturnTextures() {
    ImageSpaceEffectInterface::spMaskTexture = nullptr;
    for (int32_t i = 0; i < 5; ++i) {
        ImageSpaceTexture* ptex = pTextures.m_pBase[i];
        if (ptex) {
            ptex->SetTexture(static_cast<NiTexture*>(nullptr));
        }
    }
}

bool ImageSpaceEffectInterface::UpdateParams(ImageSpaceEffectParam* pParam) {
    ImageSpaceTexture* v4;
    ImageSpaceShaderParam* pparam;
    pTextures.m_pBase[3]->SetTexture(ImageSpaceEffectInterface::spScanlinesTexture.m_pObject);
    pTextures.m_pBase[4]->SetTexture(ImageSpaceEffectInterface::spDistortTexture.m_pObject);
    NiTexture* m_pObject = ImageSpaceEffectInterface::spMaskTexture.m_pObject;
    if (ImageSpaceEffectInterface::spMaskTexture.m_pObject) {
        v4 = pTextures.m_pBase[2];
    }
    else {
        ImageSpaceTexture** m_pBase = pTextures.m_pBase;
        m_pObject = m_pBase[1]->GetTexture();
        v4 = m_pBase[2];
    }
    v4->SetTexture(m_pObject);
    ImageSpaceTexture** v6 = pTextures.m_pBase;
    float foffsetx = 0.0f;
    float foffsety = 0.0f;
    // Unsigned-to-float sequence, as in Setup.
    if (v6[2]) {
        NiTexture* ptex = v6[2]->GetTexture();
        int32_t iWidth = static_cast<int32_t>(ptex->GetWidth());
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        foffsetx = 1.0 / dWidth * ImageSpaceEffectInterface::fBlurRadius;
        int32_t iHeight = static_cast<int32_t>(ptex->GetHeight());
        double dHeight = iHeight;
        if (iHeight < 0) {
            dHeight += 4294967296.0f;
        }
        foffsety = 1.0 / dHeight * ImageSpaceEffectInterface::fBlurRadius;
    }
    if (ImageSpaceEffectInterface::eMode) {
        double v10;
        pparam = pEffectParamList.m_pBase[1].m_pObject;
        pparam->SetPixelConstant(
            0, ImageSpaceEffectInterface::fBlurIntensity, ImageSpaceEffectInterface::fVerticalHold,
            ImageSpaceEffectInterface::fTextureUsedPart, ImageSpaceEffectInterface::fScanlineFrequency
        );
        if (ImageSpaceEffectInterface::bDistortEverywhere) {
            v10 = 1.0;
        }
        else {
            v10 = 0.0;
        }
        float v16 = v10;
        pparam->SetPixelConstant(
            1, ImageSpaceEffectInterface::fDistortVerticalScale, ImageSpaceEffectInterface::fDistortVerticalPosition,
            ImageSpaceEffectInterface::fDistortHorizontalScale, v16
        );
        pparam->SetPixelConstant(
            2, ImageSpaceEffectInterface::kTint.r, ImageSpaceEffectInterface::kTint.g,
            ImageSpaceEffectInterface::kTint.b, ImageSpaceEffectInterface::kTint.a
        );
        pparam->SetPixelConstant(3, foffsetx, foffsety, 0.0f, 0.0f);
        pparam->SetPixelConstant(4, ImageSpaceEffectInterface::fPulse, 0.0f, 0.0f, 0.0f);
    }
    else {
        double v9;
        pparam = pEffectParamList.m_pBase->m_pObject;
        pparam->SetPixelConstant(
            0, ImageSpaceEffectInterface::fBlurIntensity, ImageSpaceEffectInterface::fVerticalHold,
            ImageSpaceEffectInterface::fTextureUsedPart, ImageSpaceEffectInterface::fScanlineFrequency
        );
        if (ImageSpaceEffectInterface::bDistortEverywhere) {
            v9 = 1.0;
        }
        else {
            v9 = 0.0;
        }
        float v15 = v9;
        pparam->SetPixelConstant(
            1, ImageSpaceEffectInterface::fDistortVerticalScale, ImageSpaceEffectInterface::fDistortVerticalPosition,
            ImageSpaceEffectInterface::fDistortHorizontalScale, v15
        );
        pparam->SetPixelConstant(
            2, ImageSpaceEffectInterface::kTint.r, ImageSpaceEffectInterface::kTint.g,
            ImageSpaceEffectInterface::kTint.b, ImageSpaceEffectInterface::kTint.a
        );
        pparam->SetPixelConstant(3, foffsetx, foffsety, 0.0f, 0.0f);
    }
    pparam->iClearFlags = ImageSpaceEffectInterface::eClearFlag;
    for (uint32_t i = 0; i < pTextures.m_usSize;
         pTextures.m_pBase[i++]->SetFilterMode(NiTexturingProperty::FILTER_BILERP)) {
        ;
    }
    return false;
}

// The shared textures are released; the shader parameters in slots 0 and 1 and the textures in slots 3, 4 and 2 are
// the effect's.
ImageSpaceEffectInterface::~ImageSpaceEffectInterface() {
    spScanlinesTexture = nullptr;
    spDistortTexture = nullptr;
    spMaskTexture = nullptr;
    for (uint32_t i = 0; i < 2; ++i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
    static const uint32_t kTextureSlots[] = {3u, 4u, 2u};
    for (uint32_t uiSlot : kTextureSlots) {
        if (ImageSpaceTexture* pTexture = pTextures.m_pBase[uiSlot]) {
            pTexture->~ImageSpaceTexture();
            MemoryManager_Singleton.Deallocate(pTexture);
            pTextures.SetAt(uiSlot, nullptr);
        }
    }
}
