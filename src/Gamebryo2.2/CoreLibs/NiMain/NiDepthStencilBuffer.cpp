#include "Gamebryo2.2/CoreLibs/NiMain/NiDepthStencilBuffer.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NiRTTI NiDepthStencilBuffer::ms_RTTI("NiDepthStencilBuffer", &Ni2DBuffer::ms_RTTI);

NiDepthStencilBuffer::NiDepthStencilBuffer() {}

NiDepthStencilBuffer::~NiDepthStencilBuffer() {}

NiDepthStencilBuffer* NiDepthStencilBuffer::Create(
    uint32_t uiWidth,
    uint32_t uiHeight,
    NiRenderer* pkRenderer,
    const NiPixelFormat& kFormat,
    Ni2DBuffer::MultiSamplePreference eMSAAPref
) {
    if (kFormat.m_eFormat != NiPixelFormat::FORMAT_DEPTH_STENCIL) {
        return nullptr;
    }
    NiDepthStencilBuffer* pkBuffer_2 = new NiDepthStencilBuffer;
    pkBuffer_2->m_uiWidth = uiWidth;
    pkBuffer_2->m_uiHeight = uiHeight;
    if (pkBuffer_2->CreateRendererData(kFormat, eMSAAPref)) {
        return pkBuffer_2;
    }
    delete pkBuffer_2;
    return nullptr;
}

NiDepthStencilBuffer* NiDepthStencilBuffer::CreateCompatible(Ni2DBuffer* pkBuffer, NiRenderer* apkRenderer) {
    if (!pkBuffer || !apkRenderer) {
        return nullptr;
    }
    // The renderer parameter is the base NiRenderer; the format lookup is the DX9 renderer's.
    NiDX9Renderer* pkDX9Renderer = static_cast<NiDX9Renderer*>(apkRenderer);
    const NiPixelFormat* pkFrontFormat =
        pkBuffer->m_spRendererData.m_pObject ? pkBuffer->m_spRendererData.m_pObject->m_pkPixelFormat : nullptr;
    NiPixelFormat* pkFormat = pkDX9Renderer->FindClosestDepthStencilFormat(pkFrontFormat, 32, 8);
    Ni2DBuffer::MultiSamplePreference eMSAAPref = pkBuffer->m_spRendererData.m_pObject
        ? pkBuffer->m_spRendererData.m_pObject->m_eMSAAPref
        : Ni2DBuffer::MULTISAMPLE_NONE;
    return Create(pkBuffer->m_uiWidth, pkBuffer->m_uiHeight, apkRenderer, *pkFormat, eMSAAPref);
}

NiDepthStencilBuffer*
NiDepthStencilBuffer::Create(uint32_t uiWidth, uint32_t uiHeight, Ni2DBuffer::RendererData* pkData) {
    NiDepthStencilBuffer* v4 = new NiDepthStencilBuffer;
    v4->m_uiWidth = uiWidth;
    v4->m_uiHeight = uiHeight;
    v4->m_spRendererData = pkData;
    return v4;
}
