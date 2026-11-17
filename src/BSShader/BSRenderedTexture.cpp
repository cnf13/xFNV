#include "BSShader/BSRenderedTexture.h"

#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "SDK/MSVC/ehvec.h"

BSRenderedTexture::RenderTargetArray BSRenderedTexture::spRenderTargets;

// A render-target group per face of a cube map, else one over the texture's buffer; the depth buffer when asked for
// (abDepthBuffer's low byte).
BSRenderedTexture::BSRenderedTexture(
    NiRenderedTexture* apTexture,
    int32_t abDepthBuffer,
    NiDepthStencilBuffer* apSharedDepthBuffer
) {
    spTexture[0] = apTexture;
    NiDepthStencilBuffer* pDepth =
        static_cast<uint8_t>(abDepthBuffer) ? GetDepthStencilBuffer(apTexture, apSharedDepthBuffer) : nullptr;
    const NiRTTI* pRTTI = apTexture ? apTexture->GetRTTI() : nullptr;
    while (pRTTI && pRTTI != &NiRenderedCubeMap::ms_RTTI) {
        pRTTI = pRTTI->GetBaseRTTI();
    }
    if (pRTTI) {
        NiRenderedCubeMap* pCubeMap = static_cast<NiRenderedCubeMap*>(apTexture);
        for (int32_t i = 0; i < 6; ++i) {
            spGroup[i] =
                NiRenderTargetGroup::Create(pCubeMap->m_aspFaceBuffers[i].m_pObject, NiRenderer::ms_pkRenderer, pDepth);
        }
    }
    else {
        spGroup[0] = NiRenderTargetGroup::Create(
            apTexture->GetBuffer(), static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer), pDepth
        );
    }
    iAliasedType = -1;
}

// The groups are released before the textures; the restore groups release themselves.
BSRenderedTexture::~BSRenderedTexture() {
    for (NiPointer<NiRenderTargetGroup>& rspGroup : spGroup) {
        rspGroup = nullptr;
    }
    for (NiPointer<NiRenderedTexture>& rspTexture : spTexture) {
        rspTexture = nullptr;
    }
}

// Returns the cube-map face's group for a cube map, or the first group otherwise.
NiRenderTargetGroup* BSRenderedTexture::GetGroup() {
    if (!this) {
        return nullptr;
    }

    NiRenderedTexture* pTexture = spTexture[0].m_pObject;
    if (pTexture && pTexture->GetRTTI() == &NiRenderedCubeMap::ms_RTTI) {
        NiRenderedCubeMap* pCubeMap = static_cast<NiRenderedCubeMap*>(pTexture);
        return spGroup[pCubeMap->m_eFace].m_pObject;
    }
    return spGroup[0].m_pObject;
}

void BSRenderedTexture::ClearRenderedTextures() {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        (&spTexture[i])->operator=(nullptr);
    }
}

void BSRenderedTexture::EndAll() {
    NiRenderTargetGroup* m_pObject;
    NiPointer<NiRenderTargetGroup>* v2;
    int32_t v0 = BSRenderedTexture::iNumTargets;
    while (v0) {
        m_pObject = BSRenderedTexture::spRenderTargets[--v0].m_pObject;
        v2 = &BSRenderedTexture::spRenderTargets[v0];
        BSRenderedTexture::iNumTargets = v0;
        if (m_pObject) {
            m_pObject->DecRefCount();
            v0 = BSRenderedTexture::iNumTargets;
            v2->m_pObject = nullptr;
        }
    }
    if (NiRenderer::ms_pkRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->EndUsingRenderTargetGroup();
    }
}

// Ends the current render-target group, begins apGroup (null: the renderer's default group) with aiClearMode's
// flags and pushes it as the group End returns to.
void BSRenderedTexture::Begin(NiRenderTargetGroup* apTargetGroup, uint32_t aiClearMode) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    NiRenderTargetGroup* pGroup = apTargetGroup;
    if (!pGroup) {
        pGroup = pRenderer->GetDefaultRenderTargetGroup();
    }
    if (pRenderer->m_bRenderTargetGroupActive) {
        static_cast<NiDX9Renderer*>(pRenderer)->EndUsingRenderTargetGroup();
    }
    NiRenderTargetGroup* pTarget = pGroup;
    if (!pTarget) {
        pTarget = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
    }
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->BeginUsingRenderTargetGroup(pTarget, aiClearMode);
    Push(pGroup);
}

// Pushes apTex as the group End returns to, when fewer than ten are pushed. The compare is unsigned.
void BSRenderedTexture::Push(NiRenderTargetGroup* apTex) {
    if ((uint32_t)iNumTargets < 10) {
        NiPointer<NiRenderTargetGroup>& rspTarget = spRenderTargets[iNumTargets];
        rspTarget = apTex;
        ++iNumTargets;
    }
}

void BSRenderedTexture::Pop() {
    if (BSRenderedTexture::iNumTargets) {
        NiRenderTargetGroup* m_pObject =
            BSRenderedTexture::spRenderTargets[BSRenderedTexture::iNumTargets - 1].m_pObject;
        NiPointer<NiRenderTargetGroup>* v1 = &BSRenderedTexture::spRenderTargets[--BSRenderedTexture::iNumTargets];
        if (m_pObject) {
            NiDecRefCount(m_pObject);
            v1->m_pObject = nullptr;
        }
    }
}

NiRenderTargetGroup* BSRenderedTexture::GetCurrent() {
    if (BSRenderedTexture::iNumTargets) {
        return BSRenderedTexture::spRenderTargets[BSRenderedTexture::iNumTargets - 1].m_pObject;
    }
    return nullptr;
}

// Ends the current render-target group, pops the pushed group and begins the one before it without clearing.
void BSRenderedTexture::End() {
    if (!iNumTargets) {
        return;
    }
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->EndUsingRenderTargetGroup();
    Pop();
    NiRenderTargetGroup* pGroup = GetCurrent();
    if (!pGroup) {
        pGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
    }
    static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->BeginUsingRenderTargetGroup(pGroup, NiRenderer::CLEAR_NONE);
}

// Begins the renderer's frame when it is outside one, then begins drawing into apTarget (null: the default group) with
// aiClearMode's flags.
void BSRenderedTexture::Start(uint32_t aiClearMode, NiRenderTargetGroup* apTarget) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_OUTSIDE_FRAME &&
        static_cast<NiDX9Renderer*>(pRenderer)->Do_BeginFrame()) {
        pRenderer->m_eFrameState = NiRenderer::FRAMESTATE_INSIDE_FRAME;
    }
    Begin(apTarget, aiClearMode);
}

// Ends every target begun, then ends the renderer's frame and displays it (counting the frame) when it is inside one.
void BSRenderedTexture::Stop() {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    EndAll();
    if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME && pRenderer->Do_EndFrame()) {
        pRenderer->m_eFrameState = NiRenderer::FRAMESTATE_WAITING_FOR_DISPLAY;
    }
    if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_WAITING_FOR_DISPLAY && pRenderer->Do_DisplayFrame()) {
        ++pRenderer->m_uiFrameID;
        pRenderer->m_eFrameState = NiRenderer::FRAMESTATE_OUTSIDE_FRAME;
    }
}

// Begins an offscreen frame when the renderer is outside a frame, then begins drawing into apTarget like Start.
void BSRenderedTexture::StartOffscreen(uint32_t aiClearMode, NiRenderTargetGroup* apTarget) {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_OUTSIDE_FRAME &&
        static_cast<NiDX9Renderer*>(pRenderer)->Do_BeginFrame()) {
        pRenderer->m_eFrameState = NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME;
    }
    Begin(apTarget, aiClearMode);
}

// Ends every target begun, then the renderer's offscreen frame when it is inside one.
void BSRenderedTexture::StopOffscreen() {
    NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
    EndAll();
    if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME && pRenderer->Do_EndFrame()) {
        pRenderer->m_eFrameState = NiRenderer::FRAMESTATE_OUTSIDE_FRAME;
    }
}

// The depth/stencil buffer apTexture renders with: apSharedDepthBuffer when it is at least as large and its
// multisampling matches, else a compatible new one. A cube map compares against its first face's buffer.
NiDepthStencilBuffer*
BSRenderedTexture::GetDepthStencilBuffer(NiRenderedTexture* apTexture, NiDepthStencilBuffer* apSharedDepthBuffer) {
    const NiRTTI* pRTTI = nullptr;
    if (apTexture) {
        pRTTI = apTexture->GetRTTI();
    }
    while (pRTTI) {
        if (pRTTI == &NiRenderedCubeMap::ms_RTTI) {
            Ni2DBuffer* pFaceBuffer = static_cast<NiRenderedCubeMap*>(apTexture)->m_aspFaceBuffers[0].m_pObject;
            if (apSharedDepthBuffer && apSharedDepthBuffer->m_uiWidth >= pFaceBuffer->m_uiWidth &&
                apSharedDepthBuffer->m_uiHeight >= pFaceBuffer->m_uiHeight) {
                return apSharedDepthBuffer;
            }
            return NiDepthStencilBuffer::CreateCompatible(pFaceBuffer, BSShaderManager::pRenderer);
        }
        pRTTI = pRTTI->m_pkBaseRTTI;
    }
    if (apSharedDepthBuffer && apSharedDepthBuffer->m_uiWidth >= apTexture->GetWidth() &&
        apSharedDepthBuffer->m_uiHeight >= apTexture->GetHeight()) {
        Ni2DBuffer::MultiSamplePreference eSharedMSAA = apSharedDepthBuffer->m_spRendererData
            ? apSharedDepthBuffer->m_spRendererData->m_eMSAAPref
            : Ni2DBuffer::MULTISAMPLE_NONE;
        Ni2DBuffer* pBuffer = apTexture->GetBuffer();
        Ni2DBuffer::MultiSamplePreference eTextureMSAA =
            pBuffer->m_spRendererData ? pBuffer->m_spRendererData->m_eMSAAPref : Ni2DBuffer::MULTISAMPLE_NONE;
        if (eSharedMSAA == eTextureMSAA) {
            return apSharedDepthBuffer;
        }
    }
    return NiDepthStencilBuffer::CreateCompatible(apTexture->GetBuffer(), BSShaderManager::pRenderer);
}

// Creates the rendered texture and, when that succeeds, the render target over it.
BSRenderedTexture* BSRenderedTexture::Create(
    const NiFixedString& aName,
    uint32_t aiWidth,
    uint32_t aiHeight,
    NiTexture::FormatPrefs& arPrefs,
    Ni2DBuffer::MultiSamplePreference aeMultiSamplePreference,
    int32_t abDepthBuffer,
    NiDepthStencilBuffer* apSharedDepthBuffer,
    NiRenderedTexture* apAliasTex,
    uint32_t uiAliasOffset
) {
    NiRenderedTexture* pTexture = NiRenderedTexture::Create(
        aName, aiWidth, aiHeight, BSShaderManager::pRenderer, arPrefs, aeMultiSamplePreference, apAliasTex,
        uiAliasOffset
    );
    if (!pTexture) {
        return nullptr;
    }
    void* pMemory = NiMemObject::operator new(sizeof(BSRenderedTexture));
    if (!pMemory) {
        return nullptr;
    }
    return ::new (pMemory) BSRenderedTexture(pTexture, abDepthBuffer, apSharedDepthBuffer);
}

void BSRenderedTexture::BeginTexture(BSRenderedTexture* apTex, uint32_t aiClearMode) {
    NiRenderTargetGroup* pRTGroup;
    if (apTex) {
        pRTGroup = apTex->GetGroup();
    }
    else {
        pRTGroup = nullptr;
    }
    BSRenderedTexture::Begin(pRTGroup, aiClearMode);
}
