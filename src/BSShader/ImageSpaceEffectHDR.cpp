#include "BSShader/ImageSpaceEffectHDR.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DebugShader.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectCinematic.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

ImageSpaceTexture ImageSpaceEffectHDR::kAdaptedLight;

// Downsamples the scene texture to 1x1 in quarter steps (pass 5; with anisotropic downsampling the first step is two
// pass-7 draws through an intermediate at the device's anisotropy 8), adapts the light from the smallest texture (pass
// 11, or pass 12 while there is no adapted light yet), keeps the result as the adapted light, then runs the bright-pass
// blur and the final HDR pass. Each pass's texel size is computed from the height of the first texture it fetches and
// the width of the second.
void ImageSpaceEffectHDR::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    FNVD_INLINED_LOCAL(ImageSpaceTexture, psrc, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(ImageSpaceTexture, pdst, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(ImageSpaceTexture, pblursrc, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(ImageSpaceTexture, blurswap, std::nullopt, std::nullopt);
    // EXE-INLINED 0x4EBB70
    psrc.bForceAniso = false;
    psrc.bIsRenderedTexture = false;
    psrc.bIsTextureBorrowed = false;
    psrc.spTexture = nullptr;
    psrc.eFilterMode = 0;
    psrc.eClampMode = 0;
    // EXE-INLINED 0x4EBB70
    pdst.bForceAniso = false;
    pdst.bIsRenderedTexture = false;
    pdst.bIsTextureBorrowed = false;
    pdst.spTexture = nullptr;
    pdst.eFilterMode = 0;
    pdst.eClampMode = 0;
    // EXE-INLINED 0x4EBB70
    pblursrc.bForceAniso = false;
    pblursrc.bIsRenderedTexture = false;
    pblursrc.bIsTextureBorrowed = false;
    pblursrc.spTexture = nullptr;
    pblursrc.eFilterMode = 0;
    pblursrc.eClampMode = 0;
    // EXE-INLINED 0x4EBB70
    blurswap.bForceAniso = false;
    blurswap.bIsRenderedTexture = false;
    blurswap.bIsTextureBorrowed = false;
    blurswap.spTexture = nullptr;
    blurswap.eFilterMode = 0;
    blurswap.eClampMode = 0;
    if (bParamsChanged) {
        bParamsChanged = UpdateParams(apParam);
    }
    pdst.BorrowRenderedTexture(BSTextureManager::TEX_HDR_DOWNSAMPLE);
    psrc.SetTexture(pTextures.m_pBase[1]->GetRenderedTexture());
    int32_t iWidth = pdst.GetTexture()->GetWidth();
    int32_t iHeight = pdst.GetTexture()->GetHeight();
    NiTexturingProperty::FilterMode eFilterMode =
        BSShaderManager::bFPFiltering ? NiTexturingProperty::FILTER_BILERP : NiTexturingProperty::FILTER_NEAREST;
    if (bUseAnisoDownsample) {
        FNVD_INLINED_LOCAL(ImageSpaceTexture, kAnisoTex, std::nullopt, std::nullopt);
        // EXE-INLINED 0x4EBB70
        kAnisoTex.bForceAniso = false;
        kAnisoTex.bIsRenderedTexture = false;
        kAnisoTex.bIsTextureBorrowed = false;
        kAnisoTex.spTexture = nullptr;
        kAnisoTex.eFilterMode = 0;
        kAnisoTex.eClampMode = 0;
        ImageSpaceEffect* pEffect = pEffectList.m_pBase[7].m_pObject;
        ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[7].m_pObject;
        kAnisoTex.BorrowRenderedTexture(BSTextureManager::TEX_HDR_DOWNSAMPLE_ANISOINTERMEDIATE);
        pEffect->SetTexture(0, &kAnisoTex, NiTexturingProperty::FILTER_NEAREST);
        pEffect->SetTexture(1, &psrc, NiTexturingProperty::FILTER_MAGLERP_MINANISO);
        static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)
            ->m_pkD3DDevice9->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);
        NiTexture* pHeightTex = psrc.GetTexture();
        NiTexture* pWidthTex = psrc.GetTexture();
        int32_t iTexHeight = pHeightTex->GetHeight();
        pParam->SetTextureOffsets(0, pWidthTex->GetWidth(), iTexHeight);
        pEffect->Render(apGeometry, apRenderer, pParam, 1);
        pEffect->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
        pEffect->SetTexture(1, &kAnisoTex, NiTexturingProperty::FILTER_NEAREST);
        pHeightTex = pdst.GetTexture();
        pWidthTex = psrc.GetTexture();
        iTexHeight = pHeightTex->GetHeight();
        pParam->SetTextureOffsets(0, pWidthTex->GetWidth(), iTexHeight);
        pEffect->pTextures.m_pBase[1]->bForceAniso = true;
        pEffect->Render(apGeometry, apRenderer, pParam, 1);
        static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)
            ->m_pkD3DDevice9->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
        kAnisoTex.ReturnRenderedTexture();
        kAnisoTex.~ImageSpaceTexture();
    }
    else {
        ImageSpaceEffect* pEffect = pEffectList.m_pBase[5].m_pObject;
        ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[5].m_pObject;
        pEffect->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
        pEffect->SetTexture(1, &psrc, eFilterMode);
        NiTexture* pHeightTex = psrc.GetTexture();
        NiTexture* pWidthTex = psrc.GetTexture();
        int32_t iTexHeight = static_cast<int32_t>(pHeightTex->GetHeight());
        double dTexHeight = iTexHeight;
        if (iTexHeight < 0) {
            dTexHeight += 4294967296.0f;
        }
        float fTexelHeight = 1.0 / dTexHeight;
        int32_t iTexWidth = static_cast<int32_t>(pWidthTex->GetWidth());
        double dTexWidth = iTexWidth;
        if (iTexWidth < 0) {
            dTexWidth += 4294967296.0f;
        }
        float fTexelWidth = 1.0 / dTexWidth;
        pParam->SetPixelConstant(2, fTexelWidth, fTexelHeight, 0.0f, 0.0f);
        pEffect->Render(apGeometry, apRenderer, pParam, 1);
    }
    if (bShowDebugTextures) {
        NiTexture* pTexture = psrc.GetTexture();
        static_cast<DebugShader*>(BSShaderManager::GetShader(9))->SetTextureToRender(pTexture);
    }
    pblursrc = (psrc = pdst);
    while (iWidth > 1 || iHeight > 1) {
        iWidth /= 4;
        if (iWidth < 1) {
            iWidth = 1;
        }
        iHeight /= 4;
        if (iHeight < 1) {
            iHeight = 1;
        }
        pdst.SetTexture(
            BSShaderManager::pTextureManager->BorrowRenderedTexture(
                BSShaderManager::pRenderer.m_pObject, iWidth, iHeight, 4, D3DFMT_A16B16G16R16F, D3DFMT_UNKNOWN,
                BSTextureManager::TEX_HDR_DOWNSAMPLE, D3DMULTISAMPLE_NONE, nullptr, 0
            )
        );
        pdst.SetBorrowed(true);
        if (iHeight > 1 || iWidth > 1) {
            ImageSpaceEffect* pEffect = pEffectList.m_pBase[5].m_pObject;
            ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[5].m_pObject;
            pEffect->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
            pEffect->SetTexture(1, &psrc, eFilterMode);
            NiTexture* pHeightTex = psrc.GetTexture();
            NiTexture* pWidthTex = psrc.GetTexture();
            int32_t iTexHeight = static_cast<int32_t>(pHeightTex->GetHeight());
            double dTexHeight = iTexHeight;
            if (iTexHeight < 0) {
                dTexHeight += 4294967296.0f;
            }
            float fTexelHeight = 1.0 / dTexHeight;
            int32_t iTexWidth = static_cast<int32_t>(pWidthTex->GetWidth());
            double dTexWidth = iTexWidth;
            if (iTexWidth < 0) {
                dTexWidth += 4294967296.0f;
            }
            float fTexelWidth = 1.0 / dTexWidth;
            pParam->SetPixelConstant(2, fTexelWidth, fTexelHeight, 0.0f, 0.0f);
            pEffect->Render(apGeometry, apRenderer, pParam, 1);
            // The source goes back to the pool unless it is the scene texture or the first downsample.
            NiTexture* pScene = pTextures.m_pBase[1]->GetTexture();
            if (psrc.GetTexture() != pScene) {
                NiTexture* pFirst = pblursrc.GetTexture();
                if (psrc.GetTexture() != pFirst) {
                    psrc.ReturnRenderedTexture();
                }
            }
            psrc = pdst;
        }
    }
    ImageSpaceEffect* pEffect;
    ImageSpaceShaderParam* pParam;
    if (!kAdaptedLight.GetTexture()) {
        pEffect = pEffectList.m_pBase[12].m_pObject;
        pParam = pEffectParamList.m_pBase[12].m_pObject;
        pEffect->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
        pEffect->SetTexture(1, &psrc, eFilterMode);
        NiTexture* pHeightTex = pdst.GetTexture();
        NiTexture* pWidthTex = pdst.GetTexture();
        int32_t iTexHeight = static_cast<int32_t>(pHeightTex->GetHeight());
        double dTexHeight = iTexHeight;
        if (iTexHeight < 0) {
            dTexHeight += 4294967296.0f;
        }
        float fTexelHeight = 1.0 / dTexHeight;
        int32_t iTexWidth = static_cast<int32_t>(pWidthTex->GetWidth());
        double dTexWidth = iTexWidth;
        if (iTexWidth < 0) {
            dTexWidth += 4294967296.0f;
        }
        float fTexelWidth = 1.0 / dTexWidth;
        pParam->SetPixelConstant(2, fTexelWidth, fTexelHeight, 0.0f, 0.0f);
        pEffect->Render(apGeometry, apRenderer, pParam, 1);
    }
    else {
        pEffect = pEffectList.m_pBase[11].m_pObject;
        pParam = pEffectParamList.m_pBase[11].m_pObject;
        pEffect->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
        pEffect->SetTexture(1, &psrc, eFilterMode);
        pEffect->SetTexture(2, &kAdaptedLight, NiTexturingProperty::FILTER_NEAREST);
        NiTexture* pHeightTex = pdst.GetTexture();
        NiTexture* pWidthTex = pdst.GetTexture();
        int32_t iTexHeight = static_cast<int32_t>(pHeightTex->GetHeight());
        double dTexHeight = iTexHeight;
        if (iTexHeight < 0) {
            dTexHeight += 4294967296.0f;
        }
        float fTexelHeight = 1.0 / dTexHeight;
        int32_t iTexWidth = static_cast<int32_t>(pWidthTex->GetWidth());
        double dTexWidth = iTexWidth;
        if (iTexWidth < 0) {
            dTexWidth += 4294967296.0f;
        }
        float fTexelWidth = 1.0 / dTexWidth;
        pParam->SetPixelConstant(2, fTexelWidth, fTexelHeight, 0.0f, 0.0f);
        pEffect->Render(apGeometry, apRenderer, pParam, 1);
        kAdaptedLight.ReturnRenderedTexture();
    }
    kAdaptedLight = pdst;
    blurswap.BorrowRenderedTexture(BSTextureManager::TEX_HDR_BLURSWAP);
    // pfData[1] is the number of blur passes, [6] the bright scale, [7] the bright clamp (HDRParams' order).
    int32_t iBlurRadius = static_cast<int32_t>(pfData[1]) - 1;
    if (iBlurRadius > 6) {
        iBlurRadius = 6;
    }
    else if (iBlurRadius < 0) {
        iBlurRadius = 0;
    }
    ImageSpaceEffectBlur::fBrightClamp = pfData[7];
    ImageSpaceEffectBlur::fBrightScale = pfData[6];
    ImageSpaceEffectBlur::fBlurRadius = static_cast<float>(iBlurRadius + 1);
    ImageSpaceEffect* pBlur = BSShaderManager::pImageSpaceManager->pEffectList
                                  .m_pBase[ImageSpaceManager::EFFECT_BRIGHTPASS_BLUR3 + iBlurRadius];
    pBlur->SetTexture(0, &pblursrc, NiTexturingProperty::FILTER_NEAREST);
    pBlur->SetTexture(1, &pblursrc, NiTexturingProperty::FILTER_NEAREST);
    pBlur->SetTexture(2, &blurswap, NiTexturingProperty::FILTER_NEAREST);
    // Tests the radius again, unsigned, which the clamp above always passes.
    if (static_cast<uint32_t>(iBlurRadius) <= 6) {
        pBlur->SetTexture(3, &kAdaptedLight, NiTexturingProperty::FILTER_NEAREST);
    }
    // The blur pass takes the adaptation pass's parameters.
    pBlur->Render(apGeometry, apRenderer, pParam, 1);
    ImageSpaceEffect* pFinal;
    if (bUseAlphaMask) {
        pFinal = pEffectList.m_pBase[4].m_pObject;
    }
    else {
        pFinal = pEffectList.m_pBase[3].m_pObject;
    }
    ImageSpaceShaderParam* pFinalParam = pEffectParamList.m_pBase[2].m_pObject;
    pFinal->SetTexture(0, pTextures.m_pBase[0], NiTexturingProperty::FILTER_NEAREST);
    pFinal->SetTexture(1, &pblursrc, eFilterMode);
    pFinal->SetTexture(2, pTextures.m_pBase[1], NiTexturingProperty::FILTER_NEAREST);
    pFinal->SetTexture(3, &kAdaptedLight, NiTexturingProperty::FILTER_NEAREST);
    pFinal->Render(apGeometry, apRenderer, pFinalParam, 1);
    if (bShowDebugTextures) {
        NiTexture* pTexture = pblursrc.GetTexture();
        static_cast<DebugShader*>(BSShaderManager::GetShader(9))->SetTextureToRender(pTexture);
        pTexture = kAdaptedLight.GetTexture();
        static_cast<DebugShader*>(BSShaderManager::GetShader(9))->SetTextureToRender(pTexture);
        pTexture = blurswap.GetTexture();
        static_cast<DebugShader*>(BSShaderManager::GetShader(9))->SetTextureToRender(pTexture);
    }
    pblursrc.ReturnRenderedTexture();
    blurswap.ReturnRenderedTexture();
    NiTexture* pAdapted = kAdaptedLight.GetTexture();
    if (psrc.GetTexture() != pAdapted) {
        psrc.ReturnRenderedTexture();
    }
    blurswap.~ImageSpaceTexture();
    pblursrc.~ImageSpaceTexture();
    pdst.~ImageSpaceTexture();
    psrc.~ImageSpaceTexture();
}

// Binds the eleven HDR passes to the manager's shader effects, each with its own shader parameters: the low-res flag
// first (a screen no wider than 1280 with FP and aniso min filtering), then one parameter block per pass. The passes 5,
// 11 and 12 take the FP-filtering constant group or the plain one, pass 6 the third group. The effect indices are the
// manager's effect list slots.
void ImageSpaceEffectHDR::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(0xD);
    pTextures.SetSize(3);
    bool bLowRes = false;
    if (BSShaderManager::bFPFiltering && BSShaderManager::bAnisoMinFiltering) {
        NiRenderTargetGroup* pGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
        if (pGroup->GetWidth(0) <= 1280) {
            bLowRes = true;
        }
    }
    bUseAnisoDownsample = bLowRes;
    ImageSpaceShaderParam* pParam = nullptr;
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    if (pMemory) {
        pParam = ::new (pMemory) ImageSpaceShaderParam;
    }
    pParam->InitConstantMap(0, 0);
    pParam->bUseTexOffset[1] = false;
    pParam->SetBlurClearMode();
    pEffectParamList.SetAt(7, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    SetEffect(7, apManager->pEffectList.m_pBase[34], pParam, 0);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->bUseTexOffset[1] = false;
    pParam->SetBlurClearMode();
    ImageSpaceEffect* pEffect = nullptr;
    if (BSShaderManager::bFPFiltering) {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD48, 0x40);
        pEffect = apManager->pEffectList.m_pBase[65];
    }
    else {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD88, 0x100);
        pEffect = apManager->pEffectList.m_pBase[63];
    }
    SetEffect(5, pEffect, pParam, 0);
    pEffectParamList.SetAt(5, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->SetBlurClearMode();
    if (BSShaderManager::bFPFiltering) {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD48, 0x40);
        pEffect = apManager->pEffectList.m_pBase[66];
    }
    else {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD88, 0x100);
        pEffect = apManager->pEffectList.m_pBase[68];
    }
    SetEffect(12, pEffect, pParam, 0);
    pEffectParamList.SetAt(12, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->bUseTexOffset[2] = false;
    pParam->SetBlurClearMode();
    if (BSShaderManager::bFPFiltering) {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD48, 0x40);
        pEffect = apManager->pEffectList.m_pBase[67];
    }
    else {
        pParam->CopyConstantGroup(3, kHDRConstants11ADD88, 0x100);
        pEffect = apManager->pEffectList.m_pBase[69];
    }
    SetEffect(11, pEffect, pParam, 0);
    pEffectParamList.SetAt(11, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->bUseTexOffset[1] = false;
    pParam->SetBlurClearMode();
    pParam->CopyConstantGroup(3, kHDRConstants11ADE88, 0x90);
    SetEffect(6, apManager->pEffectList.m_pBase[64], pParam, 0);
    pEffectParamList.SetAt(6, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->bUseTexOffset[1] = false;
    pParam->SetBlurClearMode();
    SetEffect(10, apManager->pEffectList.m_pBase[71], pParam, 0);
    pEffectParamList.SetAt(10, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->bUseTexOffset[1] = false;
    pParam->bUseTexOffset[2] = false;
    pParam->SetBlurClearMode();
    SetEffect(9, apManager->pEffectList.m_pBase[70], pParam, 0);
    pEffectParamList.SetAt(9, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    pParam->SetBlurClearMode();
    SetEffect(8, apManager->pEffectList.m_pBase[62], pParam, 0);
    pEffectParamList.SetAt(8, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    SetEffect(2, apManager->pEffectList.m_pBase[58], pParam, 0);
    pEffectParamList.SetAt(2, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    SetEffect(3, apManager->pEffectList.m_pBase[59], pParam, 0);
    pEffectParamList.SetAt(3, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
    pParam = pMemory ? ::new (pMemory) ImageSpaceShaderParam : nullptr;
    pParam->InitConstantMap(0, 24);
    SetEffect(4, apManager->pEffectList.m_pBase[60], pParam, 0);
    pEffectParamList.SetAt(4, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
}

// Clears the adapted light; the body ignores this.
void ImageSpaceEffectHDR::Shutdown() {
    if (kAdaptedLight.GetTexture()) {
        kAdaptedLight.ReturnRenderedTexture();
        kAdaptedLight.ClearTexture();
    }
}

// Active when HDR is on and parameter data has been set.
bool ImageSpaceEffectHDR::IsActive() {
    return BSShaderManager::bHDR && pfData;
}

// Pushes the HDR data into the shader parameters: the target luminance and the cinematic effect's constants (pass 2),
// the adaptation step from the frame time (pass 11), the upper clamp (passes 11 and 12) and the alpha mask (passes
// 2 and 3).
bool ImageSpaceEffectHDR::UpdateParams(ImageSpaceEffectParam* apParam) {
    if (!pfData) {
        return false;
    }
    if (bResetAdaptedLight) {
        // EXE-INLINED 0xBAD4A0
        if (kAdaptedLight.GetTexture()) {
            kAdaptedLight.ReturnRenderedTexture();
            kAdaptedLight.ClearTexture();
        }
    }
    ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[2].m_pObject;
    ImageSpaceEffect* pCinematic =
        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_CINEMATIC];
    pParam->SetPixelConstant(1, pfData[4], 0.0f, 0.0f, 0.0f);
    pParam->CopyConstantGroup(19, static_cast<ImageSpaceEffectCinematic*>(pCinematic)->GetParams(), 0x40);
    pParam = pEffectParamList.m_pBase[11].m_pObject;
    // Multiplies by a double 15.0 and stores the product as a float before the double compares.
    float fAdapt = BSShaderManager::fTimerValues[1] * 15.0;
    if (fAdapt < 0.0) {
        fAdapt = 0.0f;
    }
    float fClamped = fAdapt > 1.0 ? 1.0f : fAdapt;
    pParam->SetPixelConstant(0, 0.0f, 0.0f, fClamped, 0.0f);
    pParam->SetPixelConstant(1, 0.0f, 0.0f, pfData[0], pfData[5]);
    pEffectParamList.m_pBase[12].m_pObject->SetPixelConstant(1, pfData[5], 0.0f, 0.0f, 0.0f);
    float fMask = 0.0f;
    if (bUseAlphaMask) {
        fMask = 1.0f;
    }
    pEffectParamList.m_pBase[3].m_pObject->SetPixelConstant(23, fMask, 0.0f, 0.0f, 0.0f);
    pEffectParamList.m_pBase[2].m_pObject->SetPixelConstant(23, fMask, 0.0f, 0.0f, fAlpha);
    return true;
}

void ImageSpaceEffectHDR::SetParams(float* apfData) {
    bParamsChanged = true;
    pfData = apfData;
}

// Cmd_ClearAdaptedLight calls this directly.
void ImageSpaceEffectHDR::ClearAdaptedLight() {
    if (kAdaptedLight.GetTexture()) {
        kAdaptedLight.ReturnRenderedTexture();
        kAdaptedLight.ClearTexture();
    }
}
