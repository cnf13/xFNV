#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NiDX9RenderedTextureData::NiDX9RenderedTextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer, uint32_t auiUnused)
  : NiDX9TextureData(pkTexture, pkRenderer) {}

// The renderer forgets the texture's data and the render target is released; the base does the rest.
NiDX9RenderedTextureData::~NiDX9RenderedTextureData() {
    m_pkRenderer->RemoveRenderedTextureData(static_cast<NiRenderedTexture*>(m_pkTexture));
    if (m_pkRenderTarget) {
        m_pkRenderTarget->Release();
        m_pkRenderTarget = nullptr;
    }
}

// The render target is made only for a multisampled buffer; the texture always is. The two results are ORed and the
// sign is tested. The format is copied into the renderer data before the device calls. A rendered texture's own texture
// takes D3DUSAGE_RENDERTARGET; any other takes the pool chosen by the device's Direct3D9Ex state.
const NiPixelFormat*
NiDX9RenderedTextureData::CreateSurf(const NiRenderedTexture* pkTexture, Ni2DBuffer::MultiSamplePreference eMSAAPref) {
    if (!pkTexture) {
        return nullptr;
    }
    m_uiWidth = pkTexture->GetWidth();
    m_uiHeight = pkTexture->GetHeight();
    m_uiLevels = 1;
    const NiTexture::FormatPrefs kPrefs = pkTexture->m_kFormatPrefs;
    NiPixelFormat* pkFmt = NiDX9TextureData::FindClosestPixelFormat(&kPrefs, m_pkRenderer->m_aapkTextureFormats[1]);
    m_kPixelFormat = *pkFmt;
    uint32_t uiRendererHint = pkFmt->m_uiRendererHint;
    uint32_t uiUsage = 0;
    uint32_t uiPool = D3DPOOL_DEFAULT;
    if (pkTexture->bRenderedTexture) {
        uiUsage = D3DUSAGE_RENDERTARGET;
    }
    else {
        uiPool = NiDX9Renderer::bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
    }
    if (pkTexture->bForceD3DFormat) {
        uiRendererHint = pkTexture->iForcedD3DFormat;
    }
    HRESULT eD3DRet = 0;
    if (eMSAAPref != Ni2DBuffer::MULTISAMPLE_NONE) {
        eD3DRet = m_pkRenderer->m_pkD3DDevice9->CreateRenderTarget(
            m_uiWidth,
            m_uiHeight,
            uiRendererHint,
            GetMultiSampleType(eMSAAPref),
            0,
            0,
            &m_pkRenderTarget,
            nullptr
        );
    }
    IDirect3DTexture9* pkD3DTexture = nullptr;
    eD3DRet |= m_pkRenderer->m_pkD3DDevice9
                   ->CreateTexture(m_uiWidth, m_uiHeight, 1, uiUsage, uiRendererHint, uiPool, &pkD3DTexture, nullptr);
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9RenderedTextureData::CreateSurf> Failed CreateImageSurface - %s", pcError);
        m_pkD3DTexture = nullptr;
        m_pkRenderTarget = nullptr;
        return nullptr;
    }
    m_pkD3DTexture = pkD3DTexture;
    return pkFmt;
}

// CreateSurf makes the surface and the texture. The base texture is queried for its texture interface, which the render
// target's buffer data takes.
NiDX9RenderedTextureData*
NiDX9RenderedTextureData::Create(NiRenderedTexture* apTexture, NiDX9Renderer* apRenderer, uint32_t aeMultiSample) {
    NiDX9RenderedTextureData* pThis = new NiDX9RenderedTextureData(apTexture, apRenderer, 0);
    if (!pThis->CreateSurf(apTexture, static_cast<Ni2DBuffer::MultiSamplePreference>(aeMultiSample))) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkTexture->m_pkRendererData = pThis;
    IDirect3DBaseTexture9* pkD3DBaseTex = pThis->GetD3DTexture();
    Ni2DBuffer* pkBuffer = apTexture->GetBuffer();
    // The SDK's QueryInterface out-parameter is untyped.
    void* pvD3DTex = nullptr;
    HRESULT eD3DRet = pkD3DBaseTex->QueryInterface(CLSID_IDirect3DTexture9, &pvD3DTex);
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error(
            "NiDX9RenderedTextureData::Create> Failed QueryInterface IID_IDirect3DTexture9 - %s",
            pcError
        );
        delete pThis;
        return nullptr;
    }
    IDirect3DTexture9* pkD3DTex = static_cast<IDirect3DTexture9*>(pvD3DTex);
    NiDX9TextureBufferData*
        pkBuffData = NiDX9TextureBufferData::Create(pkD3DTex, &pkBuffer, aeMultiSample, pThis->m_pkRenderTarget);
    if (pkD3DTex) {
        pkD3DTex->Release();
    }
    if (!pkBuffData) {
        NiRenderer::Error("NiDX9RenderedTextureData::Create> Failed NiDX92DBufferData::Create - %s", "");
        delete pThis;
        return nullptr;
    }
    return pThis;
}
