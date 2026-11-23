#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9PixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

// The palette is released by its own destructor, the rest by the base's: no destructor of its own.
NiDX9SourceTextureData::NiDX9SourceTextureData(NiSourceTexture* texture, NiDX9Renderer* pkRenderer)
  : NiDX9TextureData(texture, pkRenderer), m_bReplacementData(false), m_bMipmap(false), m_uiFormattedSize(0),
    m_spPalette(nullptr), m_uiLevelsSkipped(0), m_uiSourceRevID(0), m_uiPalRevID(0) {}

// Takes the level count, size and pixel format of apTexture's top level and sums every level's formatted size (six
// faces for a cube texture); false for a palettized format or a level whose description Direct3D refuses.
bool NiDX9SourceTextureData::InitializeFromD3DTexture(IDirect3DBaseTexture9* apTexture) {
    m_uiLevels = apTexture->GetLevelCount();
    m_bMipmap = m_uiLevels != 1;
    m_uiFormattedSize = 0;
    const uint32_t eType = apTexture->GetType();
    if (eType == D3DRTYPE_TEXTURE) {
        IDirect3DTexture9* pkTexture = static_cast<IDirect3DTexture9*>(apTexture);
        for (uint32_t i = 0; i < m_uiLevels; ++i) {
            D3DSURFACE_DESC kDesc;
            if (pkTexture->GetLevelDesc(i, &kDesc) < 0) {
                return false;
            }
            if (i == 0) {
                m_uiHeight = kDesc.Height;
                m_uiWidth = kDesc.Width;
                if (kDesc.Format == D3DFMT_A8P8 || kDesc.Format == D3DFMT_P8) {
                    return false;
                }
                NiPixelFormat kFormat;
                m_kPixelFormat = kFormat;
                NiDX9PixelFormat::InitFromD3DFormat(kDesc.Format, &m_kPixelFormat);
            }
            m_uiFormattedSize += (m_kPixelFormat.m_ucBitsPerPixel * kDesc.Height * kDesc.Width) >> 3;
        }
    }
    else if (eType == D3DRTYPE_CUBETEXTURE) {
        IDirect3DCubeTexture9* pkTexture = static_cast<IDirect3DCubeTexture9*>(apTexture);
        for (uint32_t i = 0; i < m_uiLevels; ++i) {
            D3DSURFACE_DESC kDesc;
            if (pkTexture->GetLevelDesc(i, &kDesc) < 0) {
                return false;
            }
            if (i == 0) {
                m_uiWidth = kDesc.Width;
                m_uiHeight = kDesc.Height;
                if (kDesc.Format == D3DFMT_A8P8 || kDesc.Format == D3DFMT_P8) {
                    return false;
                }
                NiPixelFormat kFormat;
                m_kPixelFormat = kFormat;
                NiDX9PixelFormat::InitFromD3DFormat(kDesc.Format, &m_kPixelFormat);
            }
            m_uiFormattedSize += (m_kPixelFormat.m_ucBitsPerPixel * kDesc.Height * kDesc.Width) >> 3;
        }
        m_pkD3DTexture = apTexture;
        m_uiFormattedSize = 6 * m_uiFormattedSize;
        return true;
    }
    else if (eType == D3DRTYPE_VOLUMETEXTURE) {
        IDirect3DVolumeTexture9* pkTexture = static_cast<IDirect3DVolumeTexture9*>(apTexture);
        for (uint32_t i = 0; i < m_uiLevels; ++i) {
            D3DVOLUME_DESC kDesc;
            if (pkTexture->GetLevelDesc(i, &kDesc) < 0) {
                return false;
            }
            if (i == 0) {
                m_uiHeight = kDesc.Height;
                m_uiWidth = kDesc.Width;
                if (kDesc.Format == D3DFMT_A8P8 || kDesc.Format == D3DFMT_P8) {
                    return false;
                }
                NiPixelFormat kFormat;
                m_kPixelFormat = kFormat;
                NiDX9PixelFormat::InitFromD3DFormat(kDesc.Format, &m_kPixelFormat);
            }
            m_uiFormattedSize += (m_kPixelFormat.m_ucBitsPerPixel * kDesc.Depth * kDesc.Height * kDesc.Width) >> 3;
        }
    }
    m_pkD3DTexture = apTexture;
    return true;
}

// Converts the source texture's pixel data again when it or its palette changed since the last conversion, and copies
// the result into the surfaces.
void NiDX9SourceTextureData::Update() {
    if (m_bReplacementData) {
        return;
    }
    NiPixelData* pkSrcPixels = static_cast<NiSourceTexture*>(m_pkTexture)->m_spSrcPixelData.m_pObject;
    if (!pkSrcPixels) {
        return;
    }
    bool bPaletteChanged = false;
    NiPalette* pkPalette = pkSrcPixels->m_spPalette.m_pObject;
    if (pkPalette) {
        const uint32_t uiRevID = pkPalette->m_uiRevID;
        if (m_uiPalRevID != uiRevID) {
            bPaletteChanged = true;
        }
        m_uiPalRevID = uiRevID;
        if (m_spPalette.m_pObject != pkPalette) {
            m_spPalette.SetNew(pkPalette);
            bPaletteChanged = true;
        }
    }
    if (pkSrcPixels->m_uiRevID == m_uiSourceRevID && !bPaletteChanged) {
        return;
    }
    NiPixelData* pkNewPixels = NiImageConverter::GetImageConverter()
                                   ->ConvertPixelData(*pkSrcPixels, m_kPixelFormat, pkSrcPixels, m_bMipmap);
    m_uiSourceRevID = pkSrcPixels->m_uiRevID;
    if (m_pkTexture) {
        CopyDataToSurface(pkNewPixels);
    }
    // The result is held in a smart pointer for the copy and released at once.
    if (pkNewPixels) {
        pkNewPixels->IncRefCount();
        pkNewPixels->DecRefCount();
    }
}

NiPalette* NiDX9SourceTextureData::GetPalette() {
    return m_spPalette;
}

// Copies every mipmap level above m_uiLevelsSkipped of apkPixels into the texture's surfaces; a cube or volume texture
// is left alone. A failed level is reported through the engine's empty message stub.
void NiDX9SourceTextureData::CopyDataToSurface(NiPixelData* apkPixels) {
    if (!m_pkD3DTexture) {
        return;
    }
    if (m_pkD3DTexture->GetType() == D3DRTYPE_CUBETEXTURE) {
        return;
    }
    if (m_pkD3DTexture->GetType() == D3DRTYPE_VOLUMETEXTURE) {
        return;
    }
    IDirect3DTexture9* pkD3DTexture = static_cast<IDirect3DTexture9*>(m_pkD3DTexture);
    m_uiLevels = apkPixels->m_uiMipmapLevels;
    for (uint32_t i = 0; i < m_uiLevels - m_uiLevelsSkipped; ++i) {
        IDirect3DSurface9* pkMipLevel;
        const HRESULT hResult = pkD3DTexture->GetSurfaceLevel(i, &pkMipLevel);
        if (hResult < 0) {
            NiRenderer::Error(
                "NiDX9SourceTextureData::CopyDataToSurface> Failed GetSurfaceLevel (%d)- %s",
                i,
                NiDX9ErrorString(hResult)
            );
            return;
        }
        CopyDataToSurfaceLevel(apkPixels, i + m_uiLevelsSkipped, pkMipLevel, 0);
        pkMipLevel->Release();
    }
}

// Copies every mipmap level above m_uiLevelsSkipped of the persistent renderer data into the texture's surfaces; a cube
// or volume texture is left alone. Unlike CopyDataToSurface, m_uiLevels is left as it is.
void NiDX9SourceTextureData::CopyRendererDataToSurface(const NiDX9PersistentSrcTextureRendererData* apkData) {
    if (!m_pkD3DTexture) {
        return;
    }
    if (m_pkD3DTexture->GetType() == D3DRTYPE_CUBETEXTURE) {
        return;
    }
    if (m_pkD3DTexture->GetType() == D3DRTYPE_VOLUMETEXTURE) {
        return;
    }
    IDirect3DTexture9* pkD3DTexture = static_cast<IDirect3DTexture9*>(m_pkD3DTexture);
    for (uint32_t i = 0; i < m_uiLevels - m_uiLevelsSkipped; ++i) {
        IDirect3DSurface9* pkMipLevel;
        const HRESULT hResult = pkD3DTexture->GetSurfaceLevel(i, &pkMipLevel);
        if (hResult < 0) {
            NiRenderer::Error(
                "NiDX9SourceTextureData::CopyRendererDataToSurface> Failed GetSurfaceLevel (%d)- %s",
                i,
                NiDX9ErrorString(hResult)
            );
            return;
        }
        CopyRendererDataToSurfaceLevel(apkData, i + m_uiLevelsSkipped, pkMipLevel, 0);
        pkMipLevel->Release();
    }
}

// Creates the texture for apkPixels' size and level count, dropping the top ms_uiSkipLevels levels (all but one at
// most); a texture that is not a power of two in both sizes gets one level unless the renderer allows mipmapping it.
bool NiDX9SourceTextureData::CreateSurf(const NiPixelData* apkPixels) {
    IDirect3DTexture9* pkTexture;
    const D3DFORMAT eD3DFmt = NiDX9PixelFormat::DetermineD3DFormat(m_kPixelFormat);
    const uint32_t uiWidth = *apkPixels->m_puiWidth;
    m_uiWidth = uiWidth;
    const uint32_t uiHeight = *apkPixels->m_puiHeight;
    m_uiHeight = uiHeight;
    if ((uiWidth && (uiWidth - 1 & uiWidth) == 0 && uiHeight && (uiHeight - 1 & uiHeight) == 0) ||
        (m_pkRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_TEXT) != 0) {
        m_uiLevels = apkPixels->m_uiMipmapLevels;
    }
    else {
        m_uiLevels = 1;
    }
    uint32_t uiSkip = ms_uiSkipLevels;
    const uint32_t uiLevels = m_uiLevels;
    if (uiSkip > uiLevels - 1) {
        uiSkip = uiLevels - 1;
    }
    if (uiSkip) {
        uint32_t uiSkippedWidth = m_uiWidth;
        uint32_t uiSkippedHeight = m_uiHeight;
        uint32_t i = uiSkip;
        do {
            if ((uiSkippedWidth & 1) != 0) {
                ++uiSkippedWidth;
            }
            uiSkippedWidth >>= 1;
            if ((uiSkippedHeight & 1) != 0) {
                ++uiSkippedHeight;
            }
            uiSkippedHeight >>= 1;
            --i;
        }
        while (i);
        m_uiWidth = uiSkippedWidth;
        m_uiHeight = uiSkippedHeight;
    }
    m_uiLevelsSkipped = uiSkip;
    IDirect3DDevice9* pkD3DDevice = m_pkRenderer->m_pkD3DDevice9;
    pkTexture = nullptr;
    // CreateTexture takes the format as uint32_t rather than D3DFORMAT; the enum converts implicitly.
    const HRESULT hResult = pkD3DDevice->CreateTexture(
        m_uiWidth,
        m_uiHeight,
        uiLevels - uiSkip,
        NiDX9Renderer::bUseDirect3D9Ex ? 512 : 0,
        eD3DFmt,
        !NiDX9Renderer::bUseDirect3D9Ex,
        &pkTexture,
        nullptr
    );
    if (hResult >= 0 && pkTexture) {
        m_pkD3DTexture = pkTexture;
        return true;
    }
    NiRenderer::Error(
        "NiDX9SourceTextureData::CreateSurf> Failed CreateImageSurface - %s\n",
        NiDX9ErrorString(hResult)
    );
    m_pkD3DTexture = nullptr;
    return false;
}

// Loads the texture file straight into a Direct3D texture through D3DX. For a DXT-compressed DDS it skips uiMipMapSkip
// top levels (keeping at least uiMipMapMinimum) by copying the header and the remaining levels into a new buffer. D3DX
// is then passed the file's original size, not the shortened buffer's.
bool NiDX9SourceTextureData::LoadTextureFile(
    const char* pcFilename,
    NiDX9Renderer* pkRenderer,
    NiTexture::FormatPrefs* pkPrefs,
    NiFile* pkSourceFile
) {
    if (!pkRenderer) {
        return false;
    }
    IDirect3DDevice9* pkD3DDevice = pkRenderer->m_pkD3DDevice9;
    if (!pkD3DDevice) {
        return false;
    }
    NiFile* pkFile = pkSourceFile;
    if (!pkFile) {
        char acStandardFilename[260];
        strcpy_s(acStandardFilename, 260, pcFilename);
        NiPath::Standardize(acStandardFilename);
        NiFilename kFilename(acStandardFilename);
        if (NiStricmp(kFilename.m_acExt, ".bmp") &&
            (NiStricmp(kFilename.m_acExt, ".tga") && NiStricmp(kFilename.m_acExt, ".dds"))) {
            return false;
        }

        pkFile = NiFile::GetFile(acStandardFilename, NiFile::READ_ONLY, 0x8000);
        if (!pkFile) {
            return false;
        }
    }
    if (!*pkFile) {
        delete pkFile;
        return false;
    }
    uint32_t uiOriginalSize = pkFile->GetFileSize();
    if (!uiOriginalSize) {
        delete pkFile;
        return false;
    }
    uint32_t uiSize = uiOriginalSize;
    uint8_t* pucBuffer = static_cast<uint8_t*>(_NiMalloc(uiSize));
    pkFile->Read(pucBuffer, uiSize);
    delete pkFile;
    D3DXIMAGE_INFO kSrcInfo;
    if (D3DXGetImageInfoFromFileInMemory_Thunk(pucBuffer, uiSize, &kSrcInfo) < 0 ||
        (pkPrefs->m_ePixelLayout == NiTexture::FormatPrefs::BUMPMAP &&
         kSrcInfo.Format != D3DFMT_V8U8 &&
         kSrcInfo.Format != D3DFMT_Q8W8V8U8 &&
         kSrcInfo.Format != D3DFMT_V16U16 &&
         kSrcInfo.Format != D3DFMT_Q16W16V16U16 &&
         kSrcInfo.Format != D3DFMT_CxV8U8 &&
         kSrcInfo.Format != D3DFMT_L6V5U5 &&
         kSrcInfo.Format != D3DFMT_X8L8V8U8 &&
         kSrcInfo.Format != D3DFMT_A2W10V10U10)) {
        _NiFree(pucBuffer);
        return false;
    }
    uint32_t uiHeight = kSrcInfo.Height;
    uint32_t uiWidth = kSrcInfo.Width;
    IDirect3DBaseTexture9* pkTexture = nullptr;
    HRESULT hResult;
    if (kSrcInfo.ResourceType == D3DRTYPE_TEXTURE) {
        IDirect3DTexture9* pk2DTexture = nullptr;
        // The buffer is the file's raw bytes; the DDS header follows its 4-byte magic.
        DDS_HEADER* pkHeader = reinterpret_cast<DDS_HEADER*>(pucBuffer + sizeof(uint32_t));
        if (kSrcInfo.ImageFileFormat == D3DXIFF_DDS && (pkHeader->ddspf.dwFlags & DDPF_FOURCC)) {
            uint32_t uiSkip = uiMipMapSkip;
            uint32_t uiMinimum = uiMipMapMinimum;
            // The bytes of a 4x4 block are 16 divided by this: 2 for DXT1's 8-byte blocks.
            uint32_t uiBlockDivisor = 0;
            uint32_t uiFourCC = pkHeader->ddspf.dwFourCC;
            if (uiFourCC == D3DFMT_DXT1) {
                uiBlockDivisor = 2;
            }
            else if (
                uiFourCC == D3DFMT_DXT2 || uiFourCC == D3DFMT_DXT3 || uiFourCC == D3DFMT_DXT4 || uiFourCC == D3DFMT_DXT5
            ) {
                uiBlockDivisor = 1;
            }
            if (uiBlockDivisor) {
                uint32_t uiMipCount = pkHeader->dwMipMapCount;
                uint8_t* pucSrc = pucBuffer + sizeof(uint32_t) + sizeof(DDS_HEADER);
                uint32_t uiKeptLevels = uiMipCount - uiSkip;
                if (uiKeptLevels <= uiMinimum) {
                    uiKeptLevels = uiMinimum;
                }
                if (uiMipCount <= uiSkip) {
                    uiKeptLevels = uiMipCount;
                    if (uiMipCount >= uiMinimum) {
                        uiKeptLevels = uiMinimum;
                    }
                }
                uint32_t uiLevels = uiMipCount;
                if (uiKeptLevels < uiMipCount) {
                    uint32_t uiLevelWidth = kSrcInfo.Width;
                    uint32_t uiSkipped = uiMipCount - uiKeptLevels;
                    do {
                        uint32_t uiBlocksWide = uiLevelWidth >> 2;
                        if (uiBlocksWide < 1) {
                            uiBlocksWide = 1;
                        }
                        uint32_t uiLevelHeight = kSrcInfo.Height;
                        uint32_t uiBlocksHigh = uiLevelHeight >> 2;
                        if (uiBlocksHigh < 1) {
                            uiBlocksHigh = 1;
                        }
                        uint32_t uiLevelSize = ((uiBlocksHigh * uiBlocksWide) << 4) / uiBlockDivisor;
                        pucSrc += uiLevelSize;
                        uiSize -= uiLevelSize;
                        uiLevelWidth = uiLevelWidth >> 1;
                        if (uiLevelWidth < 1) {
                            uiLevelWidth = 1;
                        }
                        kSrcInfo.Height = uiLevelHeight >> 1;
                        if (kSrcInfo.Height < 1) {
                            kSrcInfo.Height = 1;
                        }
                        --uiLevels;
                    }
                    while (--uiSkipped);
                    kSrcInfo.Width = uiLevelWidth;
                }
                if (uiLevels != pkHeader->dwMipMapCount) {
                    uint8_t* pucNewBuffer = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(uiSize));
                    rt::memcpy(pucNewBuffer, pucBuffer, sizeof(uint32_t) + sizeof(DDS_HEADER));
                    DDS_HEADER* pkNewHeader = reinterpret_cast<DDS_HEADER*>(pucNewBuffer + sizeof(uint32_t));
                    pkNewHeader->dwWidth = kSrcInfo.Width;
                    pkNewHeader->dwHeight = kSrcInfo.Height;
                    pkNewHeader->dwMipMapCount = uiLevels;
                    pkNewHeader->dwPitchOrLinearSize = kSrcInfo.Height * kSrcInfo.Width;
                    rt::memcpy(
                        pucNewBuffer + sizeof(uint32_t) + sizeof(DDS_HEADER),
                        pucSrc,
                        uiSize - (sizeof(uint32_t) + sizeof(DDS_HEADER))
                    );
                    MemoryManager::Instance().Deallocate(pucBuffer);
                    pucBuffer = pucNewBuffer;
                }
                uiSize = uiOriginalSize;
            }
        }
        hResult = D3DXCreateTextureFromFileInMemory_Thunk(pkD3DDevice, pucBuffer, uiSize, &pk2DTexture);
        pkTexture = pk2DTexture;
    }
    else if (kSrcInfo.ResourceType == D3DRTYPE_CUBETEXTURE) {
        IDirect3DCubeTexture9* pkCubeTexture = nullptr;
        hResult = D3DXCreateCubeTextureFromFileInMemory_Thunk(pkD3DDevice, pucBuffer, uiSize, &pkCubeTexture);
        pkTexture = pkCubeTexture;
    }
    else if (kSrcInfo.ResourceType == D3DRTYPE_VOLUMETEXTURE) {
        IDirect3DVolumeTexture9* pkVolumeTexture = nullptr;
        hResult = D3DXCreateVolumeTextureFromFileInMemory_Thunk(pkD3DDevice, pucBuffer, uiSize, &pkVolumeTexture);
        pkTexture = pkVolumeTexture;
    }
    else {
        hResult = D3DERR_INVALIDCALL;
    }
    _NiFree(pucBuffer);
    if (hResult < 0 || !pkTexture) {
        return false;
    }
    bool bResult = InitializeFromD3DTexture(pkTexture);
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;
    if (!bResult) {
        pkTexture->Release();
    }
    return bResult;
}

// Gamebryo's NiDX9SourceTextureData::Create: cube maps go to NiDX9SourceCubeMapData; otherwise the texture is loaded
// straight into Direct3D when allowed, else built from the persistent renderer data or from the source pixels (read
// from the file when the texture has none), with the renderer's replacement data on any failure.
NiDX9SourceTextureData* NiDX9SourceTextureData::Create(NiSourceTexture* apTexture, NiDX9Renderer* apRenderer) {
    if (NiObject::IsKindOf(&NiSourceCubeMap::ms_RTTI, apTexture)) {
        return NiDX9SourceCubeMapData::Create(static_cast<NiSourceCubeMap*>(apTexture), apRenderer);
    }
    NiPixelFormat** ppkFormats = apRenderer->m_aapkTextureFormats[0];
    NiDX9SourceTextureData* pkThis = new NiDX9SourceTextureData(apTexture, apRenderer);
    NiPersistentSrcTextureRendererData* pkPersistent = apTexture->m_spPersistentSrcRendererData.m_pObject;
    NiTexture::FormatPrefs& kPrefs = apTexture->m_kFormatPrefs;
    NiPixelData* pkSrcPixels = nullptr;
    if (!pkPersistent) {
        pkSrcPixels = apTexture->m_spSrcPixelData.m_pObject;
        if (!pkSrcPixels) {
            const char* pcFilename = apTexture->m_kPlatformFilename.GetHandle();
            if (NiTexture::bLoadDirectToRenderer &&
                apTexture->m_bLoadDirectToRendererHint &&
                apRenderer &&
                pkThis->LoadTextureFile(pcFilename, apRenderer, &kPrefs, apTexture->m_pFile)) {
                pkThis->m_pkTexture->m_pkRendererData = pkThis;
                return pkThis;
            }
            pkSrcPixels = NiImageConverter::GetImageConverter()->ReadImageFile(pcFilename, nullptr, nullptr);
        }
    }
    NiPixelFormat* pkDestFmt = nullptr;
    // The source pixels as read, held with a reference until the end.
    NiPixelData* pkHeldPixels = pkSrcPixels;
    if (pkSrcPixels) {
        NiIncRefCount(pkSrcPixels);
    }
    D3DFORMAT eD3DFmt = D3DFMT_UNKNOWN;
    if (!pkPersistent) {
        if (!pkSrcPixels) {
            pkSrcPixels = NiTexture::RendererData::GetReplacementData(apTexture, NiTexture::RendererData::FILENOTFOUND);
            // The "Failed to find texture %s\n" debug output with the file name goes to an empty function, so it is
            // omitted.
            pkThis->m_bReplacementData = true;
        }
        else {
            NiTexture::RendererData::CreationFailure eFailure = NiTexture::RendererData::NONE;
            uint32_t uiWidth = *pkSrcPixels->m_puiWidth;
            uint32_t uiHeight;
            if ((uiWidth &&
                 !(uiWidth & (uiWidth - 1)) &&
                 (uiHeight = *pkSrcPixels->m_puiHeight) != 0 &&
                 !(uiHeight & (uiHeight - 1))) ||
                (apRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_TEXT)) {
                pkDestFmt = pkThis->FindMatchingPixelFormat(pkSrcPixels->m_kPixelFormat, kPrefs, ppkFormats);
                if (!pkDestFmt) {
                    eFailure = NiTexture::RendererData::FAILEDCONVERT;
                }
            }
            else if (apRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_CONDITIONAL_TEXT) {
                if (kPrefs.m_ePixelLayout == NiTexture::FormatPrefs::COMPRESSED) {
                    NiTexture::FormatPrefs kTruePrefs = kPrefs;
                    kTruePrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
                    pkDestFmt = pkThis->FindMatchingPixelFormat(pkSrcPixels->m_kPixelFormat, kTruePrefs, ppkFormats);
                }
                else {
                    pkDestFmt = pkThis->FindMatchingPixelFormat(pkSrcPixels->m_kPixelFormat, kPrefs, ppkFormats);
                }
                if (!pkDestFmt) {
                    eFailure = NiTexture::RendererData::FAILEDCONVERT;
                }
            }
            else {
                eFailure = NiTexture::RendererData::BADDIMENSIONS;
            }
            if (eFailure != NiTexture::RendererData::NONE) {
                pkSrcPixels = NiTexture::RendererData::GetReplacementData(apTexture, eFailure);
                pkThis->m_bReplacementData = true;
            }
        }
    }
    else {
        NiTexture::RendererData::CreationFailure eFailure = NiTexture::RendererData::NONE;
        uint32_t uiWidth = *pkPersistent->m_puiWidth;
        uint32_t uiHeight;
        if ((uiWidth &&
             !(uiWidth & (uiWidth - 1)) &&
             (uiHeight = *pkPersistent->m_puiHeight) != 0 &&
             !(uiHeight & (uiHeight - 1))) ||
            (apRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_TEXT) ||
            (apRenderer->GetFlags() & NiRenderer::CAPS_NONPOW2_CONDITIONAL_TEXT)) {
            pkDestFmt = pkThis->FindMatchingPixelFormat(pkPersistent->m_kPixelFormat, kPrefs, ppkFormats);
            if (!pkDestFmt) {
                eFailure = NiTexture::RendererData::FAILEDCONVERT;
            }
        }
        else {
            eFailure = NiTexture::RendererData::BADDIMENSIONS;
        }
        if (eFailure != NiTexture::RendererData::NONE) {
            pkSrcPixels = NiTexture::RendererData::GetReplacementData(apTexture, eFailure);
            pkThis->m_bReplacementData = true;
        }
        eD3DFmt = NiDX9PixelFormat::DetermineD3DFormat(pkPersistent->m_kPixelFormat);
        if (!eD3DFmt) {
            pkSrcPixels = NiTexture::RendererData::GetReplacementData(
                apTexture,
                NiTexture::RendererData::FAILEDCONVERT
            );
            pkThis->m_bReplacementData = true;
        }
        NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
        if (NiDX9Renderer::ms_pkD3D9->CheckDeviceFormat(
                pkRenderer->m_uiAdapter,
                pkRenderer->m_eDevType,
                pkRenderer->m_eAdapterFormat,
                0,
                D3DRTYPE_TEXTURE,
                eD3DFmt
            ) < 0) {
            pkSrcPixels = NiTexture::RendererData::GetReplacementData(
                apTexture,
                NiTexture::RendererData::FAILEDCONVERT
            );
            pkThis->m_bReplacementData = true;
        }
    }
    bool bReplacement = pkThis->m_bReplacementData;
    if (bReplacement || !pkPersistent) {
        if (bReplacement) {
            pkDestFmt = ppkFormats[apRenderer->m_eReplacementDataFormat];
        }
        pkThis->m_uiSourceRevID = pkSrcPixels->m_uiRevID;
        pkThis->m_kPixelFormat = *pkDestFmt;
        pkThis->m_uiWidth = *pkSrcPixels->m_puiWidth;
        pkThis->m_uiHeight = *pkSrcPixels->m_puiHeight;
        bool bMipmap = false;
        if (!bReplacement) {
            NiTexture::FormatPrefs::MipFlag eMipMapped = apTexture->m_kFormatPrefs.m_eMipMapped;
            if (eMipMapped == NiTexture::FormatPrefs::YES ||
                (eMipMapped == NiTexture::FormatPrefs::MIP_DEFAULT && NiTexture::ms_bMipmapByDefault)) {
                bMipmap = true;
            }
        }
        pkThis->m_bMipmap = bMipmap;
        NiPixelData* pkFormatted;
        if (bReplacement) {
            pkFormatted = pkSrcPixels;
        }
        else {
            pkFormatted = NiImageConverter::GetImageConverter()
                              ->ConvertPixelData(*pkSrcPixels, *pkDestFmt, pkSrcPixels, pkThis->m_bMipmap);
        }
        if (!pkThis->m_pkD3DTexture) {
            pkThis->CreateSurf(pkFormatted);
        }
        if (pkThis->m_pkD3DTexture) {
            pkThis->CopyDataToSurface(pkFormatted);
        }
        pkThis->m_uiLevels = pkFormatted->m_uiMipmapLevels;
        NiPalette* pkPalette = pkSrcPixels->m_spPalette.m_pObject;
        if (pkPalette) {
            pkThis->m_uiPalRevID = pkPalette->m_uiRevID;
            if (pkThis->m_spPalette.m_pObject != pkPalette) {
                pkThis->m_spPalette.SetNew(pkPalette);
            }
        }
        pkThis->m_uiFormattedSize = pkFormatted->m_uiFaces *
            pkFormatted->m_puiOffsetInBytes[pkFormatted->m_uiMipmapLevels];
        // Takes and drops a reference on the formatted pixels, as a smart-pointer temporary would.
        NiIncRefCount(pkFormatted);
        NiDecRefCount(pkFormatted);
    }
    else {
        // Gamebryo's cast: the DX9 renderer's persistent data is its own subclass.
        NiDX9PersistentSrcTextureRendererData* pkDX9Persistent = static_cast<NiDX9PersistentSrcTextureRendererData*>(
            pkPersistent
        );
        pkThis->m_kPixelFormat = pkPersistent->m_kPixelFormat;
        pkThis->m_uiWidth = *pkPersistent->m_puiWidth;
        pkThis->m_uiHeight = *pkPersistent->m_puiHeight;
        uint32_t uiLevels = pkPersistent->m_uiMipmapLevels;
        pkThis->m_uiLevels = uiLevels;
        pkThis->m_bMipmap = uiLevels != 1;
        pkThis->m_bReplacementData = false;
        pkThis->m_pkRenderer = apRenderer;
        pkThis->m_uiFormattedSize = pkPersistent->m_uiFaces *
            pkPersistent->m_puiOffsetInBytes[pkPersistent->m_uiMipmapLevels];
        NiPalette* pkPalette = pkPersistent->m_spPalette.m_pObject;
        if (pkPalette) {
            pkThis->m_uiPalRevID = pkPalette->m_uiRevID;
            if (pkThis->m_spPalette.m_pObject != pkPalette) {
                pkThis->m_spPalette.SetNew(pkPalette);
            }
        }
        if (!pkThis->m_pkD3DTexture) {
            pkThis->CreateSurfFromRendererData(pkDX9Persistent, eD3DFmt);
        }
        if (pkThis->m_pkD3DTexture) {
            pkThis->CopyRendererDataToSurface(pkDX9Persistent);
        }
    }
    pkThis->m_pkTexture->m_pkRendererData = pkThis;
    if (NiSourceTexture::ms_bDestroyAppData) {
        apTexture->DestroyAppPixelData();
    }
    if (pkHeldPixels) {
        NiDecRefCount(pkHeldPixels);
    }
    return pkThis;
}
