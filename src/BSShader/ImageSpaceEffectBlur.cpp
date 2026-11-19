#include "BSShader/ImageSpaceEffectBlur.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectFullScreenBlur.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cmath>

ImageSpaceEffectBlur::ImageSpaceEffectBlur() : iRadius(-1) {
    pTextures.SetSize(4u);
}

// The two shader parameters are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectBlur::~ImageSpaceEffectBlur() {
    for (uint32_t i = 0; i < 2; ++i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
}

// The parameter list's SetAt stores the pointer without reference counting.
void ImageSpaceEffectBlur::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(2);
    pTextures.SetSize(bBrightPass ? 4u : 3u);
    int32_t iblurRadius = iRadius - 1;
    // The 2r-1 taps around entry 5 of the kernel. At radius 7 they start one entry before the kernel, on the four
    // statics iBlurTexSize's bits, fSpecularMult, fEmmisiveMult and fBrightScale, as the game does; those statics are
    // not adjacent here, so that entry is gathered explicitly.
    NiPoint4 kTaps[15];
    NiPoint4* pfdata = &kTaps[0];
    if (iblurRadius > 5) {
        rt::memcpy(&kTaps[0].x, &iBlurTexSize, sizeof(float));
        kTaps[0].y = fSpecularMult;
        kTaps[0].z = fEmmisiveMult;
        kTaps[0].w = fBrightScale;
        rt::memcpy(&kTaps[1], &pgBlurWeightsA[0], 2 * iblurRadius * sizeof(NiPoint4));
    }
    else {
        pfdata = &pgBlurWeightsA[5 - iblurRadius];
    }
    int32_t iEffect = iblurRadius + (bBrightPass ? 85 : 78);
    ImageSpaceShaderParam* pMemory =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    ImageSpaceShaderParam* pparam = nullptr;
    if (pMemory) {
        pparam = ::new (pMemory) ImageSpaceShaderParam;
    }
    pparam->InitConstantMap(0, 18);
    uint32_t uiSize = static_cast<uint32_t>((2 * iblurRadius + 1) * sizeof(NiPoint4));
    pparam->CopyConstantGroup(1, pfdata, uiSize);
    pparam->SetBlurClearMode();
    pEffectParamList.SetAt(0, NiPointer<ImageSpaceShaderParam>(pparam));
    if (bBrightPass) {
        SetEffectInputs(0, 1, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    }
    else {
        SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
    }
    SetEffect(0, apManager->pEffectList.m_pBase[iEffect], pparam, 2);
    ImageSpaceShaderParam* pMemory_2 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    ImageSpaceShaderParam* pparam_2;
    if (pMemory_2) {
        pparam_2 = ::new (pMemory_2) ImageSpaceShaderParam;
    }
    else {
        pparam_2 = nullptr;
    }
    pparam_2->InitConstantMap(0, 18);
    pparam_2->CopyConstantGroup(1, pfdata, uiSize);
    pparam_2->SetBlurClearMode();
    pEffectParamList.SetAt(1u, NiPointer<ImageSpaceShaderParam>(pparam_2));
    SetEffectInputs(1, 2, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(1, apManager->pEffectList.m_pBase[iblurRadius + 78], pparam_2, 0);
}

// Interpolates the weights of the shared kernel between the precomputed kernels of the radii around fBlurRadius,
// then sets the vertical and horizontal passes' texel steps and their 2r+1 kernel taps. Returns false.
bool ImageSpaceEffectBlur::UpdateParams(ImageSpaceEffectParam* pParam) {
    // Both sizes are converted as signed and 2^32 is added when negative, then they are stored as floats.
    NiTexture* pTexture = pTextures.m_pBase[2]->GetTexture();
    int32_t iWidth = static_cast<int32_t>(pTexture->GetWidth());
    double dWidth = iWidth;
    if (iWidth < 0) {
        dWidth += 4294967296.0f;
    }
    float fwidth = dWidth;
    NiTexture* pTexture_2 = pTextures.m_pBase[1]->GetTexture();
    int32_t iHeight = static_cast<int32_t>(pTexture_2->GetHeight());
    double dHeight = iHeight;
    if (iHeight < 0) {
        dHeight += 4294967296.0f;
    }
    float fheight = dHeight;
    int32_t iradius = iRadius;
    int32_t iindex = iradius - 1;
    int32_t iindex2 = iradius;
    if (iindex2 > 7) {
        iindex2 = 7;
    }
    if (iindex < 1) {
        iindex = 1;
    }
    float interp = 1.0 - (static_cast<double>(iradius) - ImageSpaceEffectBlur::fBlurRadius);
    if (interp == 0.0f) {
        interp = 1.0f;
    }
    NiPoint4* pLower = aBlurKernels[iindex - 1];
    NiPoint4* pUpper = aBlurKernels[iindex2 - 1];
    for (int32_t i = 0; i < 15; ++i) {
        pgBlurWeightsA[i].z = (pUpper[i].z - pLower[i].z) * interp + pLower[i].z;
    }
    ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase[0].m_pObject;
    float fInvHeight = 1.0 / fheight;
    pparam->SetPixelConstant(0, 0.0f, fInvHeight, 0.0f, 0.0f);
    if (bBrightPass) {
        pparam->SetPixelConstant(
            17, ImageSpaceEffectBlur::fBrightClamp, ImageSpaceEffectBlur::fBrightScale, 0.0f, 0.0f
        );
    }
    uint32_t uiSize = static_cast<uint32_t>((2 * iindex2 + 1) * sizeof(NiPoint4));
    NiPoint4* pTaps = &pgBlurWeightsA[7 - iindex2];
    pparam->CopyConstantGroup(1, pTaps, uiSize);
    ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase[1].m_pObject;
    float fInvWidth = 1.0 / fwidth;
    pparam_2->SetPixelConstant(0, fInvWidth, 0.0f, 0.0f, 0.0f);
    pparam_2->CopyConstantGroup(1, pTaps, uiSize);
    return false;
}

void ImageSpaceEffectBlur::SetRadius(int32_t aiRadius) {
    iRadius = aiRadius;
}

void ImageSpaceEffectBlur::SetBrightPass(bool abBrightPass) {
    bBrightPass = abBrightPass;
}

ImageSpaceEffectFullScreenBlur::ImageSpaceEffectFullScreenBlur() {
    pTextures.SetSize(4u);
    kCamDir = NiPoint3_UNIT_ALL;
}

// The eleven shader parameters are the effect's: each is deleted and its slot emptied.
ImageSpaceEffectFullScreenBlur::~ImageSpaceEffectFullScreenBlur() {
    for (uint32_t i = 0; i < 11; ++i) {
        if (pEffectParamList.m_pBase[i].m_pObject) {
            delete pEffectParamList.m_pBase[i].m_pObject;
            pEffectParamList.SetAt(i, nullptr);
        }
    }
}

void ImageSpaceEffectFullScreenBlur::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    int32_t i_2;
    BorrowTextures(apParam);
    float a2 = std::ceil(static_cast<double>(ImageSpaceEffectFullScreenBlur_fRadius));
    int32_t iradius = static_cast<int32_t>(a2);
    int32_t iradius_2 = iradius;
    if ((NiPoint3_UNIT_ALL.x != ImageSpaceEffectFullScreenBlur::kCamDir.x ||
         (NiPoint3_UNIT_ALL.y != ImageSpaceEffectFullScreenBlur::kCamDir.y ||
          NiPoint3_UNIT_ALL.z != ImageSpaceEffectFullScreenBlur::kCamDir.z)) &&
        ImageSpaceEffectFullScreenBlur::bMoving && BSShaderManager::bFakeFullScreenMotionBlur && iradius < 2) {
        iradius_2 = 2;
    }
    for (int32_t i = 0; i < 8; ++i) {
        if (i < 1 || i == iradius_2) {
            ApplyEffectTextures(i);
            UpdateParams(apParam);
            ImageSpaceEffect* peffect = pEffectList.m_pBase[i].m_pObject;
            peffect->Render(apGeometry, apRenderer, pEffectParamList.m_pBase[i].m_pObject, abEndFrame);
        }
    }
    if (!(ImageSpaceEffectFullScreenBlur_fRadius < 1.0f) ||
        (ApplyEffectTextures(10),
         (UpdateParams(apParam),
          (pEffectList.m_pBase[10]
               .m_pObject->Render(apGeometry, apRenderer, pEffectParamList.m_pBase[10].m_pObject, abEndFrame),
           !(ImageSpaceEffectFullScreenBlur_fRadius < 1.0f))))) {
        i_2 = 8;
    }
    else {
        i_2 = 9;
        BSRenderState::SetAlphaBlendEnable(true, BSRS_LOCK);
        BSRenderState::SetAlphaBlendFunc(
            NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_INVSRCALPHA, BSRS_NOLOCK
        );
    }
    ApplyEffectTextures(i_2);
    UpdateParams(apParam);
    ImageSpaceEffect* peffect_2 = pEffectList.m_pBase[i_2].m_pObject;
    peffect_2->Render(apGeometry, apRenderer, pEffectParamList.m_pBase[i_2].m_pObject, abEndFrame);
    if (ImageSpaceEffectFullScreenBlur_fRadius < 1.0f) {
        --BSRenderState::iLock[6];
        BSRenderState::SetAlphaBlendEnable(false, BSRS_NOLOCK);
    }
    ReturnTextures();
}

void ImageSpaceEffectFullScreenBlur::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    ImageSpaceShaderParam* pparam;
    SetEffectCount(11);
    ImageSpaceShaderParam* v4 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    if (v4) {
        pparam = ::new (v4) ImageSpaceShaderParam;
    }
    else {
        pparam = nullptr;
    }
    pparam->InitConstantMap(0, 24);
    pparam->bUseTexOffset[1] = false;
    pparam->CopyConstantGroup(3, kHDRConstants11ADD48, 64u);
    SetEffect(0, apManager->pEffectList.m_pBase[65], pparam, 2);
    NiPointer<ImageSpaceShaderParam> spParam(pparam);
    NiTPrimitiveArray<NiPointer<ImageSpaceShaderParam>>* p_pEffectParamList = &pEffectParamList;
    pEffectParamList.SetAt(0, spParam);
    SetEffectInputs(0, 1, NiTexturingProperty::FILTER_BILERP);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(1u, spParam);
    SetEffectInputs(1, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(1, apManager->pEffectList.m_pBase[16], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(2u, spParam);
    SetEffectInputs(2, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(2, apManager->pEffectList.m_pBase[17], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(3u, spParam);
    SetEffectInputs(3, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(3, apManager->pEffectList.m_pBase[18], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(4u, spParam);
    SetEffectInputs(4, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(4, apManager->pEffectList.m_pBase[19], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(5u, spParam);
    SetEffectInputs(5, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(5, apManager->pEffectList.m_pBase[20], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(6u, spParam);
    SetEffectInputs(6, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(6, apManager->pEffectList.m_pBase[21], nullptr, 2);
    spParam.m_pObject = nullptr;
    pEffectParamList.SetAt(7u, spParam);
    SetEffectInputs(7, 2, 3, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(7, apManager->pEffectList.m_pBase[22], nullptr, 2);
    ImageSpaceShaderParam* v8 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam.m_pObject = v8;
    if (v8) {
        spParam.m_pObject = ::new (v8) ImageSpaceShaderParam;
    }
    else {
        spParam.m_pObject = nullptr;
    }
    spParam.m_pObject->InitConstantMap(0, 24);
    spParam.m_pObject->CopyConstantGroup(3, kHDRConstants11ADD48, 64u);
    SetEffect(8, apManager->pEffectList.m_pBase[65], spParam.m_pObject, 0);
    p_pEffectParamList->SetAt(8u, spParam);
    SetEffectInputs(8, 2, NiTexturingProperty::FILTER_BILERP);
    ImageSpaceShaderParam* v9 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam.m_pObject = v9;
    if (v9) {
        spParam.m_pObject = ::new (v9) ImageSpaceShaderParam;
    }
    else {
        spParam.m_pObject = nullptr;
    }
    spParam.m_pObject->InitConstantMap(0, 24);
    spParam.m_pObject->CopyConstantGroup(3, kHDRConstants11ADD48, 64u);
    SetEffect(9, apManager->pEffectList.m_pBase[65], spParam.m_pObject, 0);
    p_pEffectParamList->SetAt(9u, spParam);
    SetEffectInputs(9, 2, NiTexturingProperty::FILTER_BILERP);
    ImageSpaceShaderParam* v10 =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    spParam.m_pObject = v10;
    if (v10) {
        spParam.m_pObject = ::new (v10) ImageSpaceShaderParam;
    }
    else {
        spParam.m_pObject = nullptr;
    }
    spParam.m_pObject->InitConstantMap(0, 24);
    SetEffect(10, apManager->pEffectList.m_pBase[34], spParam.m_pObject, 0);
    p_pEffectParamList->SetAt(10u, spParam);
    SetEffectInputs(10, 1, NiTexturingProperty::FILTER_NEAREST);
}

void ImageSpaceEffectFullScreenBlur::BorrowTextures(ImageSpaceEffectParam* pParam) {
    ImageSpaceTexture* ptex = pBufferA;
    pBufferA->BorrowRenderedTexture(BSTextureManager::TEX_BLURFULL_BUFFER);
    pTextures.SetAt(2u, ptex);
    ptex = &pBufferA[1];
    pBufferA[1].BorrowRenderedTexture(BSTextureManager::TEX_BLURFULL_BUFFER);
    pTextures.SetAt(3u, ptex);
}

// Hands back the two blur textures and clears their slots.
void ImageSpaceEffectFullScreenBlur::ReturnTextures() {
    pTextures.m_pBase[TEXTURE_BLUR1]->ReturnRenderedTexture();
    pTextures.SetAt(TEXTURE_BLUR1, nullptr);
    pTextures.m_pBase[TEXTURE_BLUR2]->ReturnRenderedTexture();
    pTextures.SetAt(TEXTURE_BLUR2, nullptr);
}

bool ImageSpaceEffectFullScreenBlur::IsActive() {
    return ((NiPoint3_UNIT_ALL.x != ImageSpaceEffectFullScreenBlur::kCamDir.x ||
             (NiPoint3_UNIT_ALL.y != ImageSpaceEffectFullScreenBlur::kCamDir.y ||
              NiPoint3_UNIT_ALL.z != ImageSpaceEffectFullScreenBlur::kCamDir.z)) &&
            (ImageSpaceEffectFullScreenBlur::bMoving && BSShaderManager::bFakeFullScreenMotionBlur)) ||
        ImageSpaceEffectFullScreenBlur_fRadius > 0.0f;
}

// Each unsigned size is converted as a signed value plus the float 2^32 when negative, as the game does; that addition
// rounds at the thread's precision control (24 bits under Direct3D), where a 64-bit conversion of the size would not.
bool ImageSpaceEffectFullScreenBlur::UpdateParams(ImageSpaceEffectParam* apParam) {
    int32_t iWidth;
    int32_t iHeight;
    double dWidth;
    float fwidth;
    ImageSpaceTexture** m_pBase = pTextures.m_pBase;
    if (*m_pBase && (*m_pBase)->GetTexture()) {
        NiTexture* Texture = (*pTextures.m_pBase)->GetTexture();
        iWidth = static_cast<int32_t>(Texture->GetWidth());
        dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        fwidth = dWidth;
        NiTexture* v5 = (*pTextures.m_pBase)->GetTexture();
        iHeight = static_cast<int32_t>(v5->GetHeight());
    }
    else {
        NiRenderTargetGroup* v8 = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
        iWidth = static_cast<int32_t>(v8->GetWidth(0));
        dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        fwidth = dWidth;
        NiRenderTargetGroup* v9 = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
        iHeight = static_cast<int32_t>(v9->GetHeight(0));
    }
    double dHeight = iHeight;
    if (iHeight < 0) {
        dHeight += 4294967296.0f;
    }
    float fheight = dHeight;
    float v11 = 1.0 / fheight;
    float v13 = 1.0 / fwidth;
    pEffectParamList.m_pBase->m_pObject->SetPixelConstant(2, v13, v11, 0.0f, 0.0f);
    pEffectParamList.m_pBase[8].m_pObject->SetPixelConstant(2, v13, v11, 0.0f, 0.0f);
    pEffectParamList.m_pBase[9].m_pObject->SetPixelConstant(2, v13, v11, ImageSpaceEffectFullScreenBlur_fRadius, 0.0f);
    ImageSpaceEffectBlur::fBlurRadius = ImageSpaceEffectFullScreenBlur_fRadius;
    return false;
}
