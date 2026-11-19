#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDegradePriority.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include <initializer_list>

NiDX9TextureData::NiDX9TextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer)
  : NiTexture::RendererData(pkTexture), m_pkRenderer(pkRenderer), m_pkD3DTexture(nullptr), m_uiLevels(0) {}

NiDX9TextureData::~NiDX9TextureData() {
    if (m_pkD3DTexture) {
        m_pkRenderer->m_pkRenderState->ClearTexture(m_pkD3DTexture);
        m_pkD3DTexture->Release();
        m_pkD3DTexture = nullptr;
    }
}

// Takes the level count, the top level's size and the pixel format from a 2D, cube or volume texture. Fails when
// the level description cannot be read or the format is palettized; any other resource type only sets the levels.
bool NiDX9TextureData::InitializeFromD3DTexture(IDirect3DBaseTexture9* apTexture) {
    m_uiLevels = apTexture->GetLevelCount();
    uint32_t eType = apTexture->GetType();
    uint32_t eFormat;
    if (eType == D3DRTYPE_TEXTURE) {
        D3DSURFACE_DESC kDesc;
        if (static_cast<IDirect3DTexture9*>(apTexture)->GetLevelDesc(0, &kDesc) < 0) {
            return false;
        }
        m_uiWidth = kDesc.Width;
        m_uiHeight = kDesc.Height;
        eFormat = kDesc.Format;
    }
    else if (eType == D3DRTYPE_CUBETEXTURE) {
        D3DSURFACE_DESC kDesc;
        if (static_cast<IDirect3DCubeTexture9*>(apTexture)->GetLevelDesc(0, &kDesc) < 0) {
            return false;
        }
        m_uiWidth = kDesc.Width;
        m_uiHeight = kDesc.Height;
        eFormat = kDesc.Format;
    }
    else if (eType == D3DRTYPE_VOLUMETEXTURE) {
        D3DVOLUME_DESC kDesc;
        if (static_cast<IDirect3DVolumeTexture9*>(apTexture)->GetLevelDesc(0, &kDesc) < 0) {
            return false;
        }
        m_uiWidth = kDesc.Width;
        m_uiHeight = kDesc.Height;
        eFormat = kDesc.Format;
    }
    else {
        return true;
    }

    if (eFormat == D3DFMT_A8P8 || eFormat == D3DFMT_P8) {
        return false;
    }
    NiDX9PixelFormat::InitFromD3DFormat(static_cast<D3DFORMAT>(eFormat), &m_kPixelFormat);
    return true;
}

// Every candidate is the renderer's format (appkDestFmts, by NiDX9Renderer::TexFormat) when it has it and the image
// converter can convert akSrcFmt to it. Bump map preferences and bump sources try the bump formats only. DXT and
// one-channel sources try their own format first, then, like RGBA, palette-alpha and multi-channel sources, the
// float formats (for a float or half first component, by its width and the component count). Those, and RGB and
// palettized sources, end with the compressed formats (when preferred) and the 16- and 32-bit chains.
NiPixelFormat* NiDX9TextureData::FindMatchingPixelFormat(
    const NiPixelFormat& akSrcFmt,
    const NiTexture::FormatPrefs& akPrefs,
    NiPixelFormat** appkDestFmts
) {
    using Prefs = NiTexture::FormatPrefs;
    const NiPixelFormat::Format eSrcFormat = akSrcFmt.m_eFormat;
    NiImageConverter* pkConverter = NiImageConverter::GetImageConverter();
    if (akSrcFmt.m_eTiling != NiPixelFormat::TILE_NONE) {
        return nullptr;
    }

    auto First = [&](std::initializer_list<NiDX9Renderer::TexFormat> akOrder) -> NiPixelFormat* {
        for (NiDX9Renderer::TexFormat eFormat : akOrder) {
            NiPixelFormat* pkFormat = appkDestFmts[eFormat];
            if (pkFormat && pkConverter->CanConvertPixelData(akSrcFmt, *pkFormat)) {
                return pkFormat;
            }
        }
        return nullptr;
    };

    if (akPrefs.m_ePixelLayout == Prefs::BUMPMAP) {
        if (eSrcFormat == NiPixelFormat::FORMAT_BUMPLUMA || eSrcFormat == NiPixelFormat::FORMAT_RGBA) {
            return First({NiDX9Renderer::TEX_BUMPLUMA888, NiDX9Renderer::TEX_BUMPLUMA556, NiDX9Renderer::TEX_BUMP88});
        }
        return First({NiDX9Renderer::TEX_BUMP88, NiDX9Renderer::TEX_BUMPLUMA888, NiDX9Renderer::TEX_BUMPLUMA556});
    }

    NiPixelFormat* pkFormat;
    switch (eSrcFormat) {
        case NiPixelFormat::FORMAT_RGB:
        case NiPixelFormat::FORMAT_PAL:
            if (akPrefs.m_ePixelLayout == Prefs::COMPRESSED &&
                (pkFormat = First({NiDX9Renderer::TEX_DXT1, NiDX9Renderer::TEX_DXT3, NiDX9Renderer::TEX_DXT5}))) {
                return pkFormat;
            }
            if (akPrefs.m_ePixelLayout == Prefs::HIGH_COLOR_16) {
                if ((pkFormat = First(
                         {NiDX9Renderer::TEX_RGB565, NiDX9Renderer::TEX_RGB555, NiDX9Renderer::TEX_RGB888,
                          NiDX9Renderer::TEX_RGBA8888}
                     ))) {
                    return pkFormat;
                }
            }
            else if (
                (pkFormat = First(
                     {NiDX9Renderer::TEX_RGB888, NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGB565,
                      NiDX9Renderer::TEX_RGB555}
                 ))
            ) {
                return pkFormat;
            }
            return First({NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444});
        case NiPixelFormat::FORMAT_DXT1:
            if (akPrefs.m_ePixelLayout != Prefs::HIGH_COLOR_16 && akPrefs.m_ePixelLayout != Prefs::TRUE_COLOR_32 &&
                (pkFormat = First({NiDX9Renderer::TEX_DXT1}))) {
                return pkFormat;
            }
            break;
        case NiPixelFormat::FORMAT_DXT3:
            if (akPrefs.m_ePixelLayout != Prefs::HIGH_COLOR_16 && akPrefs.m_ePixelLayout != Prefs::TRUE_COLOR_32 &&
                (pkFormat = First({NiDX9Renderer::TEX_DXT3}))) {
                return pkFormat;
            }
            break;
        case NiPixelFormat::FORMAT_DXT5:
            if (akPrefs.m_ePixelLayout != Prefs::HIGH_COLOR_16 && akPrefs.m_ePixelLayout != Prefs::TRUE_COLOR_32 &&
                (pkFormat = First({NiDX9Renderer::TEX_DXT5}))) {
                return pkFormat;
            }
            break;
        case NiPixelFormat::FORMAT_BUMP:
            return First({NiDX9Renderer::TEX_BUMP88, NiDX9Renderer::TEX_BUMPLUMA888, NiDX9Renderer::TEX_BUMPLUMA556});
        case NiPixelFormat::FORMAT_BUMPLUMA:
            return First({NiDX9Renderer::TEX_BUMPLUMA556, NiDX9Renderer::TEX_BUMPLUMA888, NiDX9Renderer::TEX_BUMP88});
        case NiPixelFormat::FORMAT_ONE_CHANNEL:
            if (akPrefs.m_eAlphaFmt == Prefs::SMOOTH) {
                if ((pkFormat = First({NiDX9Renderer::TEX_A8}))) {
                    return pkFormat;
                }
            }
            else if ((pkFormat = First({NiDX9Renderer::TEX_L8}))) {
                return pkFormat;
            }
            break;
        case NiPixelFormat::FORMAT_RGBA:
        case NiPixelFormat::FORMAT_PALALPHA:
        case NiPixelFormat::FORMAT_TWO_CHANNEL:
        case NiPixelFormat::FORMAT_THREE_CHANNEL:
        case NiPixelFormat::FORMAT_FOUR_CHANNEL: break;
        default: return nullptr;
    }

    NiPixelFormat::Component eComponent = NiPixelFormat::COMP_EMPTY;
    NiPixelFormat::Representation eRep = NiPixelFormat::REP_UNKNOWN;
    uint8_t ucBPC = 0;
    bool bSigned;
    if (akSrcFmt.GetComponent(0, eComponent, eRep, ucBPC, bSigned) &&
        (eRep == NiPixelFormat::REP_FLOAT || eRep == NiPixelFormat::REP_HALF)) {
        const uint32_t uiComponents = akSrcFmt.GetNumComponents();
        if (ucBPC == 16) {
            if (uiComponents < 2 && (pkFormat = First({NiDX9Renderer::TEX_R16F, NiDX9Renderer::TEX_R32F}))) {
                return pkFormat;
            }
            if (uiComponents < 3 && (pkFormat = First({NiDX9Renderer::TEX_RG32F, NiDX9Renderer::TEX_RG64F}))) {
                return pkFormat;
            }
            if ((pkFormat = First({NiDX9Renderer::TEX_RGBA64F, NiDX9Renderer::TEX_RGBA128F}))) {
                return pkFormat;
            }
        }
        else if (ucBPC == 32) {
            if (uiComponents < 2 && (pkFormat = First({NiDX9Renderer::TEX_R32F}))) {
                return pkFormat;
            }
            if (uiComponents < 3 && (pkFormat = First({NiDX9Renderer::TEX_RG64F}))) {
                return pkFormat;
            }
            if ((pkFormat = First({NiDX9Renderer::TEX_RGBA128F}))) {
                return pkFormat;
            }
            if (uiComponents < 2 && (pkFormat = First({NiDX9Renderer::TEX_R16F}))) {
                return pkFormat;
            }
            if (uiComponents < 3 && (pkFormat = First({NiDX9Renderer::TEX_RG32F}))) {
                return pkFormat;
            }
            if ((pkFormat = First({NiDX9Renderer::TEX_RGBA64F}))) {
                return pkFormat;
            }
        }
    }

    if (akPrefs.m_ePixelLayout == Prefs::COMPRESSED) {
        if (akPrefs.m_eAlphaFmt == Prefs::BINARY) {
            if ((pkFormat = First({NiDX9Renderer::TEX_DXT1, NiDX9Renderer::TEX_DXT3, NiDX9Renderer::TEX_DXT5}))) {
                return pkFormat;
            }
        }
        else if ((pkFormat = First({NiDX9Renderer::TEX_DXT3, NiDX9Renderer::TEX_DXT5, NiDX9Renderer::TEX_DXT1}))) {
            return pkFormat;
        }
    }
    if (akPrefs.m_ePixelLayout == Prefs::HIGH_COLOR_16) {
        if (akPrefs.m_eAlphaFmt == Prefs::BINARY) {
            return First({NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA8888});
        }
        return First({NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGBA5551});
    }
    if ((pkFormat = First({NiDX9Renderer::TEX_RGBA8888}))) {
        return pkFormat;
    }
    if (akPrefs.m_eAlphaFmt == Prefs::BINARY) {
        return First({NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444});
    }
    return First({NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA5551});
}

// A default pixel layout means true color and a default alpha format none. Palettized layouts take the palette
// formats first, then (like compressed ones) the DXT format for the alpha, then the 16-bit chain. Every chain
// returns the first format the renderer has; null when it has none of them.
NiPixelFormat*
NiDX9TextureData::FindClosestPixelFormat(const NiTexture::FormatPrefs* apPrefs, NiPixelFormat** appkFormats) {
    using Prefs = NiTexture::FormatPrefs;
    auto First = [appkFormats](std::initializer_list<NiDX9Renderer::TexFormat> akOrder) -> NiPixelFormat* {
        for (NiDX9Renderer::TexFormat eFormat : akOrder) {
            if (NiPixelFormat* pkFormat = appkFormats[eFormat]) {
                return pkFormat;
            }
        }
        return nullptr;
    };

    uint32_t ePixelLayout = apPrefs->m_ePixelLayout;
    uint32_t eAlphaFmt = apPrefs->m_eAlphaFmt;
    if (ePixelLayout == Prefs::PIX_DEFAULT) {
        ePixelLayout = Prefs::TRUE_COLOR_32;
    }
    if (eAlphaFmt == Prefs::ALPHA_DEFAULT) {
        eAlphaFmt = Prefs::NONE;
    }

    NiPixelFormat* pkFormat;
    const bool bPalettized = ePixelLayout == Prefs::PALETTIZED_8 || ePixelLayout == Prefs::PALETTIZED_4;
    if (bPalettized && eAlphaFmt == Prefs::NONE && (pkFormat = appkFormats[NiDX9Renderer::TEX_PAL8])) {
        return pkFormat;
    }
    if (bPalettized && (eAlphaFmt == Prefs::BINARY || eAlphaFmt == Prefs::SMOOTH) &&
        (pkFormat = appkFormats[NiDX9Renderer::TEX_PALA8])) {
        return pkFormat;
    }

    switch (ePixelLayout) {
        case Prefs::PALETTIZED_8:
        case Prefs::COMPRESSED:
        case Prefs::PALETTIZED_4:
            if (eAlphaFmt == Prefs::NONE || eAlphaFmt == Prefs::BINARY) {
                if ((pkFormat = appkFormats[NiDX9Renderer::TEX_DXT1])) {
                    return pkFormat;
                }
            }
            else if ((eAlphaFmt == Prefs::SMOOTH) && ((pkFormat = appkFormats[NiDX9Renderer::TEX_DXT3]))) {
                return pkFormat;
            }

            [[fallthrough]];
        case Prefs::HIGH_COLOR_16:
            switch (eAlphaFmt) {
                case Prefs::NONE:
                    return First(
                        {NiDX9Renderer::TEX_RGB565, NiDX9Renderer::TEX_RGB555, NiDX9Renderer::TEX_RGB888,
                         NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444}
                    );
                case Prefs::BINARY:
                    return First(
                        {NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA8888}
                    );
                case Prefs::SMOOTH: return First({NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA8888});
            }
            return nullptr;
        case Prefs::BUMPMAP:
            if ((pkFormat = appkFormats[NiDX9Renderer::TEX_BUMPLUMA888])) {
                return pkFormat;
            }
            if (eAlphaFmt == Prefs::NONE) {
                if ((pkFormat = First({NiDX9Renderer::TEX_BUMP88, NiDX9Renderer::TEX_BUMPLUMA556}))) {
                    return pkFormat;
                }
            }
            else if ((pkFormat = First({NiDX9Renderer::TEX_BUMPLUMA556, NiDX9Renderer::TEX_BUMP88}))) {
                return pkFormat;
            }
            [[fallthrough]];
        case Prefs::TRUE_COLOR_32:
            switch (eAlphaFmt) {
                case Prefs::NONE:
                    return First(
                        {NiDX9Renderer::TEX_RGB888, NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGB565,
                         NiDX9Renderer::TEX_RGB555, NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444}
                    );
                case Prefs::BINARY:
                    return First(
                        {NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444}
                    );
                case Prefs::SMOOTH: return First({NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGBA4444});
            }
            return nullptr;
        case Prefs::SINGLE_COLOR_8:
            if (eAlphaFmt == Prefs::NONE) {
                if ((pkFormat = appkFormats[NiDX9Renderer::TEX_L8])) {
                    return pkFormat;
                }
                return First(
                    {NiDX9Renderer::TEX_RGB565, NiDX9Renderer::TEX_RGB555, NiDX9Renderer::TEX_RGB888,
                     NiDX9Renderer::TEX_RGBA8888, NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444}
                );
            }
            if ((pkFormat = appkFormats[NiDX9Renderer::TEX_A8])) {
                return pkFormat;
            }
            return First({NiDX9Renderer::TEX_RGBA5551, NiDX9Renderer::TEX_RGBA4444, NiDX9Renderer::TEX_RGBA8888});
        case Prefs::SINGLE_COLOR_16:
            return First(
                {NiDX9Renderer::TEX_R16F, NiDX9Renderer::TEX_R32F, NiDX9Renderer::TEX_RG32F, NiDX9Renderer::TEX_RG64F,
                 NiDX9Renderer::TEX_RGBA64F, NiDX9Renderer::TEX_RGBA128F}
            );
        case Prefs::SINGLE_COLOR_32:
            return First(
                {NiDX9Renderer::TEX_R32F, NiDX9Renderer::TEX_RG64F, NiDX9Renderer::TEX_RGBA128F,
                 NiDX9Renderer::TEX_R16F, NiDX9Renderer::TEX_RG32F, NiDX9Renderer::TEX_RGBA64F}
            );
        case Prefs::DOUBLE_COLOR_32:
            return First(
                {NiDX9Renderer::TEX_RG32F, NiDX9Renderer::TEX_RG64F, NiDX9Renderer::TEX_RGBA64F,
                 NiDX9Renderer::TEX_RGBA128F}
            );
        case Prefs::DOUBLE_COLOR_64:
            return First(
                {NiDX9Renderer::TEX_RG64F, NiDX9Renderer::TEX_RGBA128F, NiDX9Renderer::TEX_RG32F,
                 NiDX9Renderer::TEX_RGBA64F}
            );
        case Prefs::FLOAT_COLOR_32:
        case Prefs::FLOAT_COLOR_64: return First({NiDX9Renderer::TEX_RGBA64F, NiDX9Renderer::TEX_RGBA128F});
        case Prefs::FLOAT_COLOR_128: return First({NiDX9Renderer::TEX_RGBA128F, NiDX9Renderer::TEX_RGBA64F});
    }
    return nullptr;
}

void NiDX9TextureData::ClearTextureData() {
    for (NiTexture* pkTexture = NiTexture::ms_pkHead; pkTexture; pkTexture = pkTexture->m_pkNext) {
        NiTexture::RendererData* pkData = pkTexture->m_pkRendererData;
        if (pkData) {
            pkTexture->m_pkRendererData = nullptr;
            delete pkData;
        }
    }
}

NiDX9SourceCubeMapData::NiDX9SourceCubeMapData(NiSourceCubeMap* apCubeMap, NiDX9Renderer* apRenderer)
  : NiDX9SourceTextureData(apCubeMap, apRenderer), m_uiSourceRevID(0) {}

// NiDX9SourceTextureData's destructor does all of it.
NiDX9SourceCubeMapData::~NiDX9SourceCubeMapData() {}

// Converts the cube map's source pixel data again and copies every face into the cube texture when its revision
// or its palette changed since the last conversion. Replacement data is never updated.
void NiDX9SourceCubeMapData::Update() {
    if (m_bReplacementData) {
        return;
    }

    NiSourceCubeMap* pkTex = static_cast<NiSourceCubeMap*>(m_pkTexture);
    bool bPaletteExpand = false;
    NiPixelData* pkSrcPixels = pkTex->m_spSrcPixelData.m_pObject;
    if (!pkSrcPixels) {
        return;
    }
    if (UpdatePalette(pkSrcPixels->m_spPalette.m_pObject)) {
        bPaletteExpand = true;
    }

    if (pkSrcPixels->m_uiRevID != m_uiSourceRevID || bPaletteExpand) {
        NiPixelData* pkFormatted = NiImageConverter::GetImageConverter()->ConvertPixelData(
            *pkSrcPixels, m_kPixelFormat, pkSrcPixels, m_bMipmap
        );
        m_uiSourceRevID = pkSrcPixels->m_uiRevID;
        if (m_pkD3DTexture) {
            for (uint32_t uiFace = 0; uiFace < pkFormatted->m_uiFaces; ++uiFace) {
                CopyDataToSurface(pkFormatted, static_cast<NiSourceCubeMap::FaceID>(uiFace));
            }
        }
        // Gamebryo's NiPixelDataPtr spDestructor = pkFormatted: frees the converted data unless it is held.
        if (pkFormatted) {
            NiIncRefCount(pkFormatted);
            NiDecRefCount(pkFormatted);
        }
    }
}

// The device creates the cube texture in the managed pool with dynamic usage when it was made through Direct3D9Ex,
// and in the default pool without usage otherwise. A failure reports the Direct3D error.
bool NiDX9SourceCubeMapData::CreateCubeMap() {
    const bool bUseD3D9Ex = NiDX9Renderer::bUseDirect3D9Ex;
    IDirect3DCubeTexture9* pkD3DCubeTexture = nullptr;
    HRESULT eD3DRet = m_pkRenderer->m_pkD3DDevice9->CreateCubeTexture(
        m_uiWidth, m_uiLevels, bUseD3D9Ex ? D3DUSAGE_DYNAMIC : 0, m_kPixelFormat.m_uiRendererHint,
        bUseD3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pkD3DCubeTexture, nullptr
    );
    if (eD3DRet < 0) {
        const char* pcError = NiDX9ErrorString(eD3DRet);
        NiRenderer::Error("NiDX9SourceCubeMapData::CreateCubeMap> Failed CreateCubeTexture - %s\n", pcError);
        return false;
    }
    m_pkD3DTexture = pkD3DCubeTexture;
    return true;
}

// Each level's cube map surface is released after the copy. A failed GetCubeMapSurface reports the error and stops.
void NiDX9SourceCubeMapData::CopyDataToSurface(NiPixelData* apkPixels, NiSourceCubeMap::FaceID aeFace) {
    IDirect3DCubeTexture9* pkCubeMap = static_cast<IDirect3DCubeTexture9*>(m_pkD3DTexture);
    for (uint32_t uiLevel = 0; uiLevel < m_uiLevels; ++uiLevel) {
        IDirect3DSurface9* pkMipLevel;
        HRESULT eD3DRet = pkCubeMap->GetCubeMapSurface(aeFace, uiLevel, &pkMipLevel);
        if (eD3DRet < 0) {
            const char* pcError = NiDX9ErrorString(eD3DRet);
            NiRenderer::Error(
                "NiDX9SourceCubeMapData::CopyDataToSurface> Failed GetSurfaceLevel (%d)- %s", uiLevel, pcError
            );
            return;
        }
        NiDX9SourceTextureData::CopyDataToSurfaceLevel(apkPixels, uiLevel, pkMipLevel, aeFace);
        pkMipLevel->Release();
    }
}

// The same loop for the persistent renderer data's faces.
void NiDX9SourceCubeMapData::CopyRendererDataToCubeMapFace(
    const NiDX9PersistentSrcTextureRendererData* apkData,
    NiSourceCubeMap::FaceID aeFace
) {
    IDirect3DCubeTexture9* pkCubeMap = static_cast<IDirect3DCubeTexture9*>(m_pkD3DTexture);
    for (uint32_t uiLevel = 0; uiLevel < m_uiLevels; ++uiLevel) {
        IDirect3DSurface9* pkMipLevel;
        HRESULT eD3DRet = pkCubeMap->GetCubeMapSurface(aeFace, uiLevel, &pkMipLevel);
        if (eD3DRet < 0) {
            const char* pcError = NiDX9ErrorString(eD3DRet);
            NiRenderer::Error(
                "NiDX9SourceCubeMapData::CopyDataToSurface> Failed GetSurfaceLevel (%d)- %s", uiLevel, pcError
            );
            return;
        }
        NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel(apkData, uiLevel, pkMipLevel, aeFace);
        pkMipLevel->Release();
    }
}

bool NiDX9SourceCubeMapData::UpdatePalette(NiPalette* a2) {
    bool result = false;
    if (a2) {
        uint32_t m_uiRevID = a2->m_uiRevID;
        result = m_uiPalRevID != m_uiRevID;
        m_uiPalRevID = m_uiRevID;
        NiPointer<NiPalette>* p_m_spPalette = &m_spPalette;
        if (p_m_spPalette->m_pObject != a2) {
            p_m_spPalette->SetNew(a2);
            return true;
        }
    }
    return result;
}

// Creates the renderer data of a cube map from its persistent renderer data or its source pixel data (converted to a
// renderer format), or from replacement data where they are missing, not square powers of two or not convertible. Also
// does the texture memory accounting (ms_uiCubeMapTextureMemory, and the bytes rounding it up to 4 KB pages in
// ms_uiCubeMapPageWaste) and refuses converted pixel data without six faces.
NiDX9SourceCubeMapData* NiDX9SourceCubeMapData::Create(NiSourceCubeMap* apCubeMap, NiDX9Renderer* apRenderer) {
    NiPixelFormat** ppkPixFmts = apRenderer->m_aapkTextureFormats[NiDX9Renderer::TEXUSE_CUBE];
    uint32_t i;
    for (i = 0; i < NiDX9Renderer::TEX_NUM; i++) {
        if (ppkPixFmts[i]) {
            break;
        }
    }
    if (i == NiDX9Renderer::TEX_NUM) {
        NiRenderer::Error("NiDX9SourceCubeMapData::Create> No Cube Map texture formats supported\n");
        return nullptr;
    }

    NiDX9SourceCubeMapData* pkThis = new NiDX9SourceCubeMapData(apCubeMap, apRenderer);

    const NiTexture::FormatPrefs& kPrefs = apCubeMap->m_kFormatPrefs;
    NiPixelData* pkSrcPixels = nullptr;
    NiDX9PersistentSrcTextureRendererData* pkPersistent =
        static_cast<NiDX9PersistentSrcTextureRendererData*>(apCubeMap->m_spPersistentSrcRendererData.m_pObject);
    if (!pkPersistent) {
        pkSrcPixels = apCubeMap->m_spSrcPixelData.m_pObject;
        if (!pkSrcPixels) {
            const char* pcFilename = apCubeMap->m_kPlatformFilename.GetHandle();
            if (pcFilename) {
                pkSrcPixels = NiImageConverter::GetImageConverter()->ReadImageFile(pcFilename, nullptr, nullptr);
            }
            else {
                pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFILENOTFOUND);
                pkThis->m_bReplacementData = true;
            }
        }
    }

    NiPixelFormat* pkDestFmt = nullptr;
    if (!pkPersistent && !pkSrcPixels) {
        pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFILENOTFOUND);
        pkThis->m_bReplacementData = true;
        NiRenderer::Error("Failed to find texture %s\n", apCubeMap->m_kPlatformFilename.GetHandle());
    }
    else if (pkPersistent) {
        NiPixelFormat& kSrcFmt = pkPersistent->m_kPixelFormat;
        const uint32_t uiWidth = *pkPersistent->m_puiWidth;
        if (uiWidth != *pkPersistent->m_puiHeight || uiWidth == 0 || (uiWidth & (uiWidth - 1)) != 0) {
            pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPBADDIMENSIONS);
            pkThis->m_bReplacementData = true;
        }
        else {
            pkDestFmt = pkThis->FindMatchingPixelFormat(kSrcFmt, kPrefs, ppkPixFmts);
            if (!pkDestFmt) {
                pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFAILEDCONVERT);
                pkThis->m_bReplacementData = true;
            }
        }

        if (!pkThis->m_bReplacementData) {
            const D3DFORMAT eD3DFmt = NiDX9PixelFormat::DetermineD3DFormat(kSrcFmt);
            if (eD3DFmt == D3DFMT_UNKNOWN) {
                pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFAILEDCONVERT);
                pkThis->m_bReplacementData = true;
            }
            else {
                // Gamebryo's (NiDX9Renderer*)NiDX9Renderer::GetRenderer().
                NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
                HRESULT eD3DRet = NiDX9Renderer::ms_pkD3D9->CheckDeviceFormat(
                    pkRenderer->m_uiAdapter, pkRenderer->m_eDevType, pkRenderer->m_eAdapterFormat, 0, D3DRTYPE_TEXTURE,
                    eD3DFmt
                );
                if (eD3DRet < 0) {
                    pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFAILEDCONVERT);
                    pkThis->m_bReplacementData = true;
                }
                else {
                    kSrcFmt.m_uiRendererHint = eD3DFmt;
                }
            }
        }
    }
    else {
        const NiPixelFormat& kSrcFmt = pkSrcPixels->m_kPixelFormat;
        const uint32_t uiWidth = *pkSrcPixels->m_puiWidth;
        if (uiWidth != *pkSrcPixels->m_puiHeight || uiWidth == 0 || (uiWidth & (uiWidth - 1)) != 0) {
            pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPBADDIMENSIONS);
            pkThis->m_bReplacementData = true;
        }
        else {
            pkDestFmt = pkThis->FindMatchingPixelFormat(kSrcFmt, kPrefs, ppkPixFmts);
            if (!pkDestFmt || kSrcFmt.m_eFormat == NiPixelFormat::FORMAT_PAL ||
                kSrcFmt.m_eFormat == NiPixelFormat::FORMAT_PALALPHA) {
                pkSrcPixels = GetReplacementData(apCubeMap, CUBEMAPFAILEDCONVERT);
                pkThis->m_bReplacementData = true;
            }
        }
    }

    if (pkThis->m_bReplacementData) {
        pkDestFmt = ppkPixFmts[apRenderer->m_eReplacementDataFormat];
        pkPersistent = nullptr;
    }

    if (pkPersistent) {
        pkThis->m_kPixelFormat = pkPersistent->m_kPixelFormat;
        pkThis->m_uiWidth = *pkPersistent->m_puiWidth;
        pkThis->m_uiHeight = *pkPersistent->m_puiHeight;
        pkThis->m_uiLevels = apRenderer->m_bMipmapCubeMaps ? pkPersistent->m_uiMipmapLevels : 1;
        pkThis->m_bMipmap = pkThis->m_uiLevels != 1;
        pkThis->m_bReplacementData = false;
        pkThis->m_pkRenderer = apRenderer;
        pkThis->UpdatePalette(pkPersistent->m_spPalette.m_pObject);
        pkThis->m_uiFormattedSize =
            pkPersistent->m_puiOffsetInBytes[pkPersistent->m_uiMipmapLevels] * pkPersistent->m_uiFaces;

        if (!pkThis->CreateCubeMap() || !pkThis->m_pkD3DTexture) {
            delete pkThis;
            return nullptr;
        }
        const uint32_t uiSize = pkThis->m_uiFormattedSize;
        ms_uiCubeMapTextureMemory += uiSize;
        uint32_t uiPageRest = 0;
        if ((uiSize & 0xFFFFF000u) != uiSize) {
            uiPageRest = (uiSize & 0xFFFFF000u) - uiSize + 0x1000;
        }
        ms_uiCubeMapPageWaste += uiPageRest;

        for (i = 0; i < NiSourceCubeMap::FACE_NUM; i++) {
            pkThis->CopyRendererDataToCubeMapFace(pkPersistent, static_cast<NiSourceCubeMap::FaceID>(i));
        }
    }
    else {
        NiPixelData* pkFormatted;
        if (pkThis->m_bReplacementData) {
            pkFormatted = pkSrcPixels;
        }
        else {
            pkFormatted = NiImageConverter::GetImageConverter()->ConvertPixelData(
                *pkSrcPixels, *pkDestFmt, pkSrcPixels, pkThis->m_bMipmap
            );
        }
        if (!pkFormatted) {
            pkFormatted = GetReplacementData(apCubeMap, CUBEMAPFAILEDCONVERT);
            pkDestFmt = ppkPixFmts[apRenderer->m_eReplacementDataFormat];
        }

        pkThis->m_uiSourceRevID = pkSrcPixels->m_uiRevID;
        pkThis->m_kPixelFormat = *pkDestFmt;
        pkThis->m_uiWidth = pkThis->m_uiHeight = *pkFormatted->m_puiWidth;
        pkThis->m_bMipmap = (!pkThis->m_bReplacementData && kPrefs.m_eMipMapped == NiTexture::FormatPrefs::YES) ||
            (kPrefs.m_eMipMapped == NiTexture::FormatPrefs::MIP_DEFAULT && NiTexture::ms_bMipmapByDefault &&
             apRenderer->m_bMipmapCubeMaps);
        pkThis->m_uiLevels = apRenderer->m_bMipmapCubeMaps ? pkFormatted->m_uiMipmapLevels : 1;

        if (!pkThis->CreateCubeMap() || !pkThis->m_pkD3DTexture ||
            pkFormatted->m_uiFaces != NiSourceCubeMap::FACE_NUM) {
            delete pkThis;
            return nullptr;
        }
        for (i = 0; i < NiSourceCubeMap::FACE_NUM; i++) {
            pkThis->CopyDataToSurface(pkFormatted, static_cast<NiSourceCubeMap::FaceID>(i));
        }
        pkThis->UpdatePalette(pkSrcPixels->m_spPalette.m_pObject);

        const uint32_t uiSize = pkFormatted->m_puiOffsetInBytes[pkFormatted->m_uiMipmapLevels] * pkFormatted->m_uiFaces;
        pkThis->m_uiFormattedSize = uiSize;
        ms_uiCubeMapTextureMemory += uiSize;
        uint32_t uiPageRest = 0;
        if ((uiSize & 0xFFFFF000u) != uiSize) {
            uiPageRest = (uiSize & 0xFFFFF000u) - uiSize + 0x1000;
        }
        ms_uiCubeMapPageWaste += uiPageRest;

        // Gamebryo's NiPixelDataPtr spDestructor = pkFormatted: frees the converted data unless it is held.
        NiIncRefCount(pkFormatted);
        NiDecRefCount(pkFormatted);
    }

    pkThis->m_pkTexture->m_pkRendererData = pkThis;
    return pkThis;
}

// Creates the texture of format aeD3DFmt, taking the persistent renderer data's level count when the texture is a power
// of two on both axes (or the device renders to non-power-of-two textures) and 1 level otherwise, then dropping the top
// ms_uiSkipLevels levels, halving the size for each.
bool NiDX9SourceTextureData::CreateSurfFromRendererData(
    const NiDX9PersistentSrcTextureRendererData* apkData,
    D3DFORMAT aeD3DFmt
) {
    const bool bPowerOf2 =
        m_uiWidth != 0 && (m_uiWidth & (m_uiWidth - 1)) == 0 && m_uiHeight != 0 && (m_uiHeight & (m_uiHeight - 1)) == 0;
    if (bPowerOf2 || (m_pkRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_TEXT) != 0) {
        m_uiLevels = apkData->m_uiMipmapLevels;
    }
    else {
        m_uiLevels = 1;
    }
    m_bMipmap = m_uiLevels != 1;
    uint32_t uiSkipLevels = ms_uiSkipLevels;
    if (uiSkipLevels > m_uiLevels - 1) {
        uiSkipLevels = m_uiLevels - 1;
    }
    for (uint32_t i = 0; i < uiSkipLevels; ++i) {
        if ((m_uiWidth & 1) != 0) {
            ++m_uiWidth;
        }
        m_uiWidth >>= 1;
        if ((m_uiHeight & 1) != 0) {
            ++m_uiHeight;
        }
        m_uiHeight >>= 1;
    }
    m_uiLevelsSkipped = uiSkipLevels;
    IDirect3DTexture9* pkTexture = nullptr;
    IDirect3DDevice9* pkD3DDevice9 = m_pkRenderer->m_pkD3DDevice9;
    const bool bUseDirect3D9Ex = NiDX9Renderer::bUseDirect3D9Ex;
    const HRESULT hResult = pkD3DDevice9->CreateTexture(
        m_uiWidth, m_uiHeight, m_uiLevels - uiSkipLevels, bUseDirect3D9Ex ? 0x200u : 0u,
        static_cast<uint32_t>(aeD3DFmt), bUseDirect3D9Ex ? 0u : 1u, &pkTexture, nullptr
    );
    if (hResult < 0 || !pkTexture) {
        NiRenderer::Error(
            "NiDX9SourceTextureData::CreateSurfFromRendererData> FAILED to create texture of format: "
            "0x%x, D3D error: %s",
            aeD3DFmt, NiDX9ErrorString(hResult)
        );
        m_pkD3DTexture = nullptr;
        return false;
    }
    m_pkD3DTexture = pkTexture;
    return true;
}

// Copies mipmap level auiLevel of face auiFace of the persistent renderer data into the locked surface apkSurface, one
// row at a time when the surface's pitch differs from the level's row size; compressed levels are measured in blocks.
void NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel(
    const NiDX9PersistentSrcTextureRendererData* apkData,
    uint32_t auiLevel,
    IDirect3DSurface9* apkSurface,
    uint32_t auiFace
) {
    D3DSURFACE_DESC kSurfDesc;
    HRESULT hResult = apkSurface->GetDesc(&kSurfDesc);
    if (hResult < 0) {
        NiRenderer::Error(
            "NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel> Failed GetDesc - %s", NiDX9ErrorString(hResult)
        );
        return;
    }
    D3DLOCKED_RECT kLockRect;
    hResult = apkSurface->LockRect(&kLockRect, nullptr, 0);
    if (hResult < 0) {
        NiRenderer::Error(
            "NiDX9SourceTextureData::CopyRendererDataToSurfaceLevel> Failed GetSurfaceLevel - %s",
            NiDX9ErrorString(hResult)
        );
        return;
    }
    uint32_t uiSrcRowSize = apkData->m_puiWidth[auiLevel] * apkData->m_uiPixelStride;
    uint32_t uiDestRowSize = static_cast<uint32_t>(kLockRect.Pitch);
    uint32_t uiMinNumRows = apkData->m_puiHeight[auiLevel];
    if (uiMinNumRows > kSurfDesc.Height) {
        uiMinNumRows = kSurfDesc.Height;
    }
    const NiPixelFormat::Format eFormat = apkData->m_kPixelFormat.m_eFormat;
    if (eFormat >= NiPixelFormat::FORMAT_DXT1 && eFormat <= NiPixelFormat::FORMAT_DXT5) {
        uiDestRowSize >>= 2;
        const uint32_t uiWidth = apkData->m_puiWidth[auiLevel];
        if (kSurfDesc.Format == D3DFMT_DXT1) {
            uiSrcRowSize = ((uiWidth + 3) >> 1) & 0x7FFFFFE;
        }
        else {
            uiSrcRowSize = (uiWidth + 3) & 0xFFFFFFC;
        }
        uiMinNumRows = (uiMinNumRows + 3) & 0xFFFFFFC;
    }
    if (uiDestRowSize != uiSrcRowSize) {
        const uint32_t uiMinRowSize = uiSrcRowSize > uiDestRowSize ? uiDestRowSize : uiSrcRowSize;
        const uint8_t* pucSrc = apkData->m_pucPixels + apkData->m_puiOffsetInBytes[auiLevel] +
            auiFace * apkData->m_puiOffsetInBytes[apkData->m_uiMipmapLevels];
        uint8_t* pucDest = static_cast<uint8_t*>(kLockRect.pBits);
        for (uint32_t y = 0; y < uiMinNumRows; ++y) {
            rt::memcpy(pucDest, pucSrc, uiMinRowSize);
            pucSrc += uiSrcRowSize;
            pucDest += kLockRect.Pitch;
        }
    }
    else {
        const uint32_t uiDataSize = uiSrcRowSize * uiMinNumRows;
        rt::memcpy(
            kLockRect.pBits,
            apkData->m_pucPixels + apkData->m_puiOffsetInBytes[auiLevel] +
                auiFace * apkData->m_puiOffsetInBytes[apkData->m_uiMipmapLevels],
            uiDataSize
        );
    }
    apkSurface->UnlockRect();
}

// Copies mipmap level auiLevel of face auiFace of apkPixels into the locked surface apkMipLevel, one row at a time when
// the surface's pitch differs from the level's row size; compressed levels are measured in blocks.
void NiDX9SourceTextureData::CopyDataToSurfaceLevel(
    NiPixelData* apkPixels,
    uint32_t auiLevel,
    IDirect3DSurface9* apkMipLevel,
    uint32_t auiFace
) {
    D3DSURFACE_DESC kSurfDesc;
    HRESULT hResult = apkMipLevel->GetDesc(&kSurfDesc);
    if (hResult < 0) {
        NiRenderer::Error(
            "NiDX9SourceTextureData::CopyDataToSurfaceLevel> Failed GetDesc - %s", NiDX9ErrorString(hResult)
        );
        return;
    }
    D3DLOCKED_RECT kLockRect;
    hResult = apkMipLevel->LockRect(&kLockRect, nullptr, 0);
    if (hResult < 0) {
        NiRenderer::Error(
            "NiDX9SourceTextureData::CopyDataToSurfaceLevel> Failed GetSurfaceLevel - %s", NiDX9ErrorString(hResult)
        );
        return;
    }
    uint32_t uiSrcRowSize = apkPixels->m_puiWidth[auiLevel] * apkPixels->m_uiPixelStride;
    uint32_t uiDestRowSize = static_cast<uint32_t>(kLockRect.Pitch);
    uint32_t uiMinNumRows = apkPixels->m_puiHeight[auiLevel];
    if (uiMinNumRows > kSurfDesc.Height) {
        uiMinNumRows = kSurfDesc.Height;
    }
    const NiPixelFormat::Format eFormat = apkPixels->m_kPixelFormat.m_eFormat;
    if (eFormat >= NiPixelFormat::FORMAT_DXT1 && eFormat <= NiPixelFormat::FORMAT_DXT5) {
        uiDestRowSize >>= 2;
        const uint32_t uiWidth = apkPixels->m_puiWidth[auiLevel];
        if (kSurfDesc.Format == D3DFMT_DXT1) {
            uiSrcRowSize = ((uiWidth + 3) >> 1) & 0x7FFFFFE;
        }
        else {
            uiSrcRowSize = (uiWidth + 3) & 0xFFFFFFC;
        }
        uiMinNumRows = (uiMinNumRows + 3) & 0xFFFFFFC;
    }
    if (uiDestRowSize != uiSrcRowSize) {
        const uint32_t uiMinRowSize = uiSrcRowSize > uiDestRowSize ? uiDestRowSize : uiSrcRowSize;
        const uint8_t* pucSrc = apkPixels->m_pucPixels + apkPixels->m_puiOffsetInBytes[auiLevel] +
            auiFace * apkPixels->m_puiOffsetInBytes[apkPixels->m_uiMipmapLevels];
        uint8_t* pucDest = static_cast<uint8_t*>(kLockRect.pBits);
        for (uint32_t y = 0; y < uiMinNumRows; ++y) {
            rt::memcpy(pucDest, pucSrc, uiMinRowSize);
            pucSrc += uiSrcRowSize;
            pucDest += kLockRect.Pitch;
        }
    }
    else {
        const uint32_t uiDataSize = uiSrcRowSize * uiMinNumRows;
        rt::memcpy(
            kLockRect.pBits,
            apkPixels->m_pucPixels + apkPixels->m_puiOffsetInBytes[auiLevel] +
                auiFace * apkPixels->m_puiOffsetInBytes[apkPixels->m_uiMipmapLevels],
            uiDataSize
        );
    }
    apkMipLevel->UnlockRect();
}
