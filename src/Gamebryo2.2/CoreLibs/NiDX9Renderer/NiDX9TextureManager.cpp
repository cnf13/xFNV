#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureManager.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9DynamicTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

NiDX9TextureManager::NiDX9TextureManager(NiDX9Renderer* pkRenderer)
  : m_pkD3DDevice9(pkRenderer->m_pkD3DDevice9), m_pkRenderer(pkRenderer) {
    m_pkD3DDevice9->AddRef();
}

NiDX9TextureManager::~NiDX9TextureManager() {
    m_pkD3DDevice9->Release();
    m_pkD3DDevice9 = nullptr;
}

// The D3D texture of apTexture, creating the renderer data of a source texture that has none (arbChanged). Rendered
// and dynamic textures return theirs as is. A source texture without a D3D texture is updated (arbChanged); one
// that is not static is updated anyway. arbMipmap: more than one level; arbNonPow2: a side is not a power of two.
IDirect3DBaseTexture9* NiDX9TextureManager::PrepareTextureForRendering(
    NiTexture* apTexture,
    bool& arbChanged,
    bool& arbMipmap,
    bool& arbNonPow2
) {
    arbChanged = false;
    arbMipmap = false;
    arbNonPow2 = false;
    if (!apTexture) {
        return nullptr;
    }

    NiCriticalSection& kLock = NiRenderer::ms_pkRenderer->m_kSourceDataCriticalSection;
    kLock.Lock();
    NiDX9TextureData* pkData = static_cast<NiDX9TextureData*>(apTexture->m_pkRendererData);
    if (!pkData) {
        NiSourceTexture* pkSource = static_cast<NiSourceTexture*>(
            apTexture->DynamicCastMember(&NiSourceTexture::ms_RTTI)
        );
        if (!pkSource) {
            kLock.Unlock();
            return nullptr;
        }
        pkData = NiDX9SourceTextureData::Create(pkSource, m_pkRenderer);
        arbChanged = true;
    }
    kLock.Unlock();

    uint32_t uiWidth = pkData->m_uiWidth;
    uint32_t uiHeight = pkData->m_uiHeight;
    bool bPow2 = uiWidth && !(uiWidth & (uiWidth - 1)) && uiHeight && !(uiHeight & (uiHeight - 1));
    arbNonPow2 = !bPow2;

    NiDX9TextureData* pkOther = pkData->GetAsRenderedTexture();
    if (!pkOther) {
        pkOther = pkData->GetAsDynamicTexture();
    }
    if (pkOther) {
        return pkOther->GetD3DTexture();
    }

    NiDX9SourceTextureData* pkSourceData = pkData->GetAsSourceTexture();
    if (pkSourceData) {
        if (!pkData->GetD3DTexture()) {
            pkSourceData->Update();
            arbChanged = true;
        }
        else if (!static_cast<NiSourceTexture*>(apTexture)->m_bStatic) {
            pkSourceData->Update();
        }
    }
    arbMipmap = pkData->GetLevels() > 1;
    return pkData->GetD3DTexture();
}

// Creates the renderer data of a source texture that has none. Fails for a null texture, a texture with no data that
// is not a source texture, and a rendered or dynamic texture without a D3D texture.
bool NiDX9TextureManager::PrecacheTexture(NiTexture* apTexture, int32_t abForceLoad, int32_t abLocked) {
    if (!apTexture) {
        return false;
    }

    NiCriticalSection& kLock = NiRenderer::ms_pkRenderer->m_kSourceDataCriticalSection;
    kLock.Lock();
    NiDX9TextureData* pkData = static_cast<NiDX9TextureData*>(apTexture->m_pkRendererData);
    if (!pkData) {
        NiSourceTexture* pkSource = static_cast<NiSourceTexture*>(
            apTexture->DynamicCastMember(&NiSourceTexture::ms_RTTI)
        );
        if (!pkSource) {
            kLock.Unlock();
            return false;
        }
        pkData = NiDX9SourceTextureData::Create(pkSource, m_pkRenderer);
    }
    kLock.Unlock();

    NiDX9TextureData* pkOther = pkData->GetAsRenderedTexture();
    if (!pkOther) {
        pkOther = pkData->GetAsDynamicTexture();
    }
    if (pkOther) {
        return pkOther->GetD3DTexture() != nullptr;
    }
    return true;
}

// The D3DFORMAT of the texture's renderer data, creating it for a source texture; D3DFMT_X8R8G8B8 when there is none.
uint32_t NiDX9TextureManager::GetFormatFromTexture(NiTexture* apTexture) {
    if (apTexture->m_pkRendererData) {
        return apTexture->m_pkRendererData->m_kPixelFormat.m_uiExtraData;
    }

    NiObject* pkSource = apTexture->DynamicCastMember(&NiSourceTexture::ms_RTTI);
    if (pkSource &&
        m_pkRenderer->CreateSourceTextureRendererData(static_cast<NiSourceTexture*>(pkSource)) &&
        apTexture->m_pkRendererData) {
        return apTexture->m_pkRendererData->m_kPixelFormat.m_uiExtraData;
    }
    return D3DFMT_X8R8G8B8;
}
