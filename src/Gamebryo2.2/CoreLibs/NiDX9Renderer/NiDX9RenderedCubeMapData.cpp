#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedCubeMapData.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NiDX9RenderedCubeMapData::NiDX9RenderedCubeMapData(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer)
  : NiDX9RenderedTextureData(apCubeMap, apRenderer, 0) {}

// The renderer forgets the cube map's data; the base does the rest.
NiDX9RenderedCubeMapData::~NiDX9RenderedCubeMapData() {
    m_pkRenderer->RemoveRenderedCubeMapData(static_cast<NiRenderedCubeMap*>(m_pkTexture));
}

// The width sets both dimensions and the level count is one. The cube texture is created in the default pool with
// render target usage; the pixel format is not copied into the renderer data here.
NiPixelFormat* NiDX9RenderedCubeMapData::CreateSurf(NiRenderedCubeMap* pkTexture) {
    if (!pkTexture) {
        return nullptr;
    }
    m_uiWidth = pkTexture->GetWidth();
    m_uiHeight = m_uiWidth;
    m_uiLevels = 1;
    const NiTexture::FormatPrefs kPrefs = pkTexture->m_kFormatPrefs;
    NiPixelFormat* pkFmt = NiDX9TextureData::FindClosestPixelFormat(&kPrefs, m_pkRenderer->m_aapkTextureFormats[3]);
    uint32_t uiRendererHint = pkFmt->m_uiRendererHint;
    if (pkTexture->bForceD3DFormat) {
        uiRendererHint = pkTexture->iForcedD3DFormat;
    }
    IDirect3DCubeTexture9* pkD3DCubeTexture = nullptr;
    HRESULT eD3DRet = m_pkRenderer->m_pkD3DDevice9->CreateCubeTexture(
        m_uiWidth,
        1,
        D3DUSAGE_RENDERTARGET,
        uiRendererHint,
        D3DPOOL_DEFAULT,
        &pkD3DCubeTexture,
        nullptr
    );
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9RenderedCubeMapData::CreateSurf> Failed CreateImageSurface - %s", pcError);
        m_pkD3DTexture = nullptr;
        return nullptr;
    }
    m_pkD3DTexture = pkD3DCubeTexture;
    return pkFmt;
}

// The constructor sets the cube map's vtable; CreateSurf makes the cube texture, then every face gets its buffer data
// from the face buffer the cube map holds. The two statistics count the six faces' bytes and the page rounding the
// allocator adds.
NiDX9RenderedCubeMapData* NiDX9RenderedCubeMapData::Create(NiRenderedCubeMap* apCubeMap, NiDX9Renderer* apRenderer) {
    NiDX9RenderedCubeMapData* pThis = new NiDX9RenderedCubeMapData(apCubeMap, apRenderer);
    NiPixelFormat* pkFmt = pThis->CreateSurf(apCubeMap);
    if (!pkFmt) {
        delete pThis;
        return nullptr;
    }
    pThis->m_pkTexture->m_pkRendererData = pThis;
    NiDX9TextureBufferData* pkFirstBuffData = nullptr;
    for (uint32_t uiFace = 0; uiFace < NiRenderedCubeMap::FACE_NUM; ++uiFace) {
        pThis->GetD3DTexture();
        Ni2DBuffer* pkBuffer = apCubeMap->m_aspFaceBuffers[uiFace].m_pObject;
        NiDX9TextureBufferData* pkBuffData = NiDX9TextureBufferData::CreateCubeFace(
            static_cast<IDirect3DCubeTexture9*>(pThis->GetD3DTexture()),
            static_cast<D3DCUBEMAP_FACES>(uiFace),
            pkFirstBuffData,
            apRenderer->m_pkD3DDevice9,
            pkBuffer
        );
        if (!pkBuffData) {
            NiRenderer::Error("NiDX9RenderedCubeMapData::Create> NiDX9TextureBufferData::CreateCubeFace - %s", "");
            delete pThis;
            return nullptr;
        }
        if (!pkFirstBuffData) {
            pkFirstBuffData = pkBuffData;
        }
    }
    const uint32_t uiSize = 6 * pThis->m_uiWidth * pThis->m_uiHeight * (pkFmt->m_ucBitsPerPixel >> 3);
    ms_uiRenderedCubeMapMemory += uiSize;
    uint32_t uiWaste = 0;
    if ((uiSize & 0xFFFFF000) != uiSize) {
        uiWaste = (uiSize & 0xFFFFF000) - uiSize + 0x1000;
    }
    ms_uiRenderedCubeMapPageWaste += uiWaste;
    return pThis;
}
