// d3dx9's texture functions as the game calls them:
// - D3DXGetImageInfoFromFileInMemory and D3DXCreate{,Cube,Volume}TextureFromFileInMemory for DDS files (the game's
//   textures are all DDS; NiDX9SourceTextureData::LoadTextureFile is the one caller). The non-Ex forms take d3dx9's
//   defaults: the file's size rounded up to a power of two, a full mip chain (levels the file lacks are made with the
//   box filter from the level above, as D3DXFilterTexture does), the file's format or the nearest one the device
//   takes, D3DPOOL_MANAGED, usage 0. Other image files (BMP, TGA, PNG, JPG...) fail with D3DXERR_INVALIDDATA.
// - D3DXLoadSurfaceFromSurface for BSShaderUtil::ConvertTexture: whole surfaces of the same size, converted through
//   float RGBA (DXT decoded, or encoded by this library's own DXT compressor).
// - D3DXSaveTextureToFileA for MapMenu's local map pictures: level 0 of a 2D texture as a PNG.

#include "D3DX9Internal.h"

#include <cstring>

#include "zlib.h"

using namespace d3dx;

namespace {

    // dds.h's flags.
    constexpr uint32_t DDS_MAGIC = 0x20534444;
    constexpr uint32_t DDSD_DEPTH = 0x800000;
    constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
    constexpr uint32_t DDPF_ALPHA = 0x2;
    constexpr uint32_t DDPF_RGB = 0x40;
    constexpr uint32_t DDPF_LUMINANCE = 0x20000;
    constexpr uint32_t DDSCAPS2_CUBEMAP = 0x200;
    constexpr uint32_t DDSCAPS2_VOLUME = 0x200000;

    constexpr uint32_t D3DPTEXTURECAPS_MIPMAP = 0x4000;
    constexpr uint32_t D3DPTEXTURECAPS_MIPVOLUMEMAP = 0x8000;
    constexpr uint32_t D3DPTEXTURECAPS_MIPCUBEMAP = 0x10000;

    // A parsed DDS file: its info and where its pixels start. A cube map holds each face's mip chain in turn, a volume
    // each level's slices in turn.
    struct DdsFile {
        D3DXIMAGE_INFO info;
        uint32_t faces;
        const uint8_t* pixels;
    };

    // The Direct3D format of a DDS pixel format, D3DFMT_UNKNOWN when the stand-in has none for it. An alpha-only DDS
    // (A8) is refused, as d3dx9_38 refuses it.
    uint32_t DdsFormat(const DDS_PIXELFORMAT& arFormat) {
        if (arFormat.dwFlags & DDPF_FOURCC) {
            return arFormat.dwFourCC;
        }
        const bool bAlpha = arFormat.dwFlags & (DDPF_ALPHAPIXELS | DDPF_ALPHA);
        static constexpr uint32_t kPacked[] = {
            D3DFMT_A8R8G8B8,
            D3DFMT_X8R8G8B8,
            D3DFMT_A8B8G8R8,
            D3DFMT_X8B8G8R8,
            D3DFMT_R8G8B8,
            D3DFMT_R5G6B5,
            D3DFMT_X1R5G5B5,
            D3DFMT_A1R5G5B5,
            D3DFMT_A4R4G4B4,
            D3DFMT_X4R4G4B4,
            D3DFMT_L8,
            D3DFMT_A8L8
        };
        for (uint32_t uiFormat : kPacked) {
            const FormatInfo& rInfo = *FindFormat(uiFormat);
            if (rInfo.bits != arFormat.dwRGBBitCount ||
                rInfo.luminance != static_cast<bool>(arFormat.dwFlags & DDPF_LUMINANCE)) {
                continue;
            }
            const bool bColor = arFormat.dwFlags & (DDPF_RGB | DDPF_LUMINANCE);
            if (bColor != (rInfo.rMask != 0)) {
                continue;
            }
            if (bColor &&
                (rInfo.rMask != arFormat.dwRBitMask ||
                 rInfo.gMask != arFormat.dwGBitMask ||
                 rInfo.bMask != arFormat.dwBBitMask)) {
                continue;
            }
            if ((bAlpha ? arFormat.dwABitMask : 0) != rInfo.aMask) {
                continue;
            }
            return uiFormat;
        }
        return D3DFMT_UNKNOWN;
    }

    uint32_t Smaller(uint32_t auiSize, uint32_t auiLevel) {
        const uint32_t uiSize = auiSize >> auiLevel;
        return uiSize ? uiSize : 1;
    }

    uint32_t LevelBytes(const FormatInfo& arInfo, uint32_t auiWidth, uint32_t auiHeight, uint32_t auiDepth) {
        return RowBytes(arInfo, auiWidth) * RowCount(arInfo, auiHeight) * auiDepth;
    }

    HRESULT ParseDds(const void* apData, uint32_t auiSize, DdsFile& arFile) {
        if (!apData || auiSize < 4 + sizeof(DDS_HEADER)) {
            return D3DXERR_INVALIDDATA;
        }
        const uint8_t* pData = static_cast<const uint8_t*>(apData);
        uint32_t uiMagic;
        std::memcpy(&uiMagic, pData, 4);
        if (uiMagic != DDS_MAGIC) {
            if (!std::memcmp(pData, "BM", 2) ||
                !std::memcmp(pData, "\x89PNG", 4) ||
                !std::memcmp(pData, "\xFF\xD8", 2)) {
                Log("image file is not DDS (BMP, PNG and JPG are not supported)");
            }
            return D3DXERR_INVALIDDATA;
        }
        DDS_HEADER kHeader;
        std::memcpy(&kHeader, pData + 4, sizeof(kHeader));
        if (kHeader.dwSize != sizeof(DDS_HEADER) || kHeader.ddspf.dwSize != sizeof(DDS_PIXELFORMAT)) {
            return D3DXERR_INVALIDDATA;
        }
        D3DXIMAGE_INFO& rInfo = arFile.info;
        rInfo.Width = kHeader.dwWidth;
        rInfo.Height = kHeader.dwHeight;
        const bool bVolume = (kHeader.dwCubemapFlags & DDSCAPS2_VOLUME) && (kHeader.dwHeaderFlags & DDSD_DEPTH);
        rInfo.Depth = bVolume && kHeader.dwDepth ? kHeader.dwDepth : 1;
        rInfo.MipLevels = kHeader.dwMipMapCount ? kHeader.dwMipMapCount : 1;
        rInfo.Format = DdsFormat(kHeader.ddspf);
        rInfo.ResourceType = (kHeader.dwCubemapFlags & DDSCAPS2_CUBEMAP)
            ? D3DRTYPE_CUBETEXTURE
            : (bVolume ? D3DRTYPE_VOLUMETEXTURE : D3DRTYPE_TEXTURE);
        rInfo.ImageFileFormat = D3DXIFF_DDS;
        arFile.faces = rInfo.ResourceType == D3DRTYPE_CUBETEXTURE ? 6 : 1;
        arFile.pixels = pData + 4 + sizeof(DDS_HEADER);
        const FormatInfo* pFormat = FindFormat(rInfo.Format);
        if (!pFormat || !rInfo.Width || !rInfo.Height) {
            Log("DDS format 0x%08X is not supported", rInfo.Format);
            return D3DXERR_INVALIDDATA;
        }
        uint64_t ulBytes = 0;
        for (uint32_t i = 0; i < rInfo.MipLevels; ++i) {
            ulBytes += LevelBytes(*pFormat, Smaller(rInfo.Width, i), Smaller(rInfo.Height, i), Smaller(rInfo.Depth, i));
        }
        if (4 + sizeof(DDS_HEADER) + ulBytes * arFile.faces > auiSize) {
            return D3DXERR_INVALIDDATA;
        }
        return D3D_OK;
    }

    uint32_t NextPow2(uint32_t auiSize) {
        uint32_t uiPow2 = 1;
        while (uiPow2 < auiSize) {
            uiPow2 <<= 1;
        }
        return uiPow2;
    }

    // The format d3dx9 creates for a file format: the file's own when the device takes it for that resource type, else
    // the nearest one with the same channels; D3DFMT_UNKNOWN when none.
    uint32_t ChooseFormat(IDirect3DDevice9* apDevice, uint32_t auiResourceType, uint32_t auiFormat) {
        IDirect3D9* pD3D = nullptr;
        D3DDEVICE_CREATION_PARAMETERS kParameters;
        D3DDISPLAYMODE kMode;
        if (apDevice->GetDirect3D(&pD3D) < 0 ||
            apDevice->GetCreationParameters(&kParameters) < 0 ||
            apDevice->GetDisplayMode(0, &kMode) < 0) {
            if (pD3D) {
                pD3D->Release();
            }
            return D3DFMT_UNKNOWN;
        }
        auto Takes = [&](uint32_t auiCandidate) {
            return FindFormat(auiCandidate) &&
                pD3D->CheckDeviceFormat(
                    kParameters.AdapterOrdinal,
                    kParameters.DeviceType,
                    kMode.Format,
                    0,
                    auiResourceType,
                    auiCandidate
                ) >= 0;
        };
        uint32_t uiChosen = D3DFMT_UNKNOWN;
        if (Takes(auiFormat)) {
            uiChosen = auiFormat;
        }
        else {
            const FormatInfo* pInfo = FindFormat(auiFormat);
            const bool bAlpha = pInfo && (pInfo->aMask || pInfo->block);
            const uint32_t uiNearest[] = {bAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8};
            for (uint32_t uiCandidate : uiNearest) {
                if (Takes(uiCandidate)) {
                    uiChosen = uiCandidate;
                    break;
                }
            }
        }
        pD3D->Release();
        return uiChosen;
    }

    // Locks one level (one cube face's level) of any texture type for writing.
    struct LevelLock {
        IDirect3DBaseTexture9* texture;
        uint32_t type;

        HRESULT Lock(uint32_t auiFace, uint32_t auiLevel, uint8_t*& arBits, uint32_t& arPitch, uint32_t& arSlicePitch) {
            HRESULT hResult;
            if (type == D3DRTYPE_VOLUMETEXTURE) {
                D3DLOCKED_BOX kBox;
                hResult = static_cast<IDirect3DVolumeTexture9*>(texture)->LockBox(auiLevel, &kBox, nullptr, 0);
                arBits = static_cast<uint8_t*>(kBox.pBits);
                arPitch = static_cast<uint32_t>(kBox.RowPitch);
                arSlicePitch = static_cast<uint32_t>(kBox.SlicePitch);
                return hResult;
            }
            D3DLOCKED_RECT kRect;
            if (type == D3DRTYPE_CUBETEXTURE) {
                hResult = static_cast<IDirect3DCubeTexture9*>(texture)->LockRect(auiFace, auiLevel, &kRect, nullptr, 0);
            }
            else {
                hResult = static_cast<IDirect3DTexture9*>(texture)->LockRect(auiLevel, &kRect, nullptr, 0);
            }
            arBits = static_cast<uint8_t*>(kRect.pBits);
            arPitch = static_cast<uint32_t>(kRect.Pitch);
            arSlicePitch = 0;
            return hResult;
        }

        void Unlock(uint32_t auiFace, uint32_t auiLevel) {
            if (type == D3DRTYPE_VOLUMETEXTURE) {
                static_cast<IDirect3DVolumeTexture9*>(texture)->UnlockBox(auiLevel);
            }
            else if (type == D3DRTYPE_CUBETEXTURE) {
                static_cast<IDirect3DCubeTexture9*>(texture)->UnlockRect(auiFace, auiLevel);
            }
            else {
                static_cast<IDirect3DTexture9*>(texture)->UnlockRect(auiLevel);
            }
        }
    };

    Image DecodeLevel(
        const FormatInfo& arInfo,
        const uint8_t* apSrc,
        uint32_t auiWidth,
        uint32_t auiHeight,
        uint32_t auiDepth
    ) {
        Image kImage;
        kImage.width = auiWidth;
        kImage.height = auiHeight;
        kImage.depth = auiDepth;
        kImage.pixels.resize(static_cast<size_t>(auiWidth) * auiHeight * auiDepth);
        const uint32_t uiPitch = RowBytes(arInfo, auiWidth);
        const uint32_t uiSlice = uiPitch * RowCount(arInfo, auiHeight);
        for (uint32_t z = 0; z < auiDepth; ++z) {
            DecodeSlice(arInfo, apSrc + z * uiSlice, uiPitch, kImage, z);
        }
        return kImage;
    }

    std::vector<uint8_t> EncodeLevel(const FormatInfo& arInfo, const Image& arImage, bool abDither) {
        const uint32_t uiPitch = RowBytes(arInfo, arImage.width);
        const uint32_t uiSlice = uiPitch * RowCount(arInfo, arImage.height);
        std::vector<uint8_t> kBytes(static_cast<size_t>(uiSlice) * arImage.depth);
        for (uint32_t z = 0; z < arImage.depth; ++z) {
            EncodeSlice(arInfo, arImage, z, kBytes.data() + z * uiSlice, uiPitch, abDither);
        }
        return kBytes;
    }

    HRESULT CreateTextureFromDds(
        IDirect3DDevice9* apDevice,
        const void* apData,
        uint32_t auiSize,
        uint32_t auiResourceType,
        IDirect3DBaseTexture9** appTexture
    ) {
        if (!apDevice || !appTexture) {
            return D3DERR_INVALIDCALL;
        }
        *appTexture = nullptr;
        DdsFile kFile;
        HRESULT hResult = ParseDds(apData, auiSize, kFile);
        if (hResult < 0) {
            return hResult;
        }
        const D3DXIMAGE_INFO& rInfo = kFile.info;
        if (rInfo.ResourceType != auiResourceType) {
            Log("DDS resource type %u where %u was asked for", rInfo.ResourceType, auiResourceType);
            return D3DXERR_INVALIDDATA;
        }
        // D3DX_DEFAULT sizes: the file's, rounded up to powers of two; a cube map's faces are square.
        const uint32_t uiWidth = NextPow2(rInfo.Width);
        const uint32_t uiHeight = auiResourceType == D3DRTYPE_CUBETEXTURE ? uiWidth : NextPow2(rInfo.Height);
        const uint32_t uiDepth = NextPow2(rInfo.Depth);
        D3DCAPS9 kCaps;
        if (apDevice->GetDeviceCaps(&kCaps) < 0) {
            return D3DERR_INVALIDCALL;
        }
        const uint32_t uiMaxSize = auiResourceType == D3DRTYPE_VOLUMETEXTURE ? kCaps.MaxVolumeExtent
                                                                             : kCaps.MaxTextureWidth;
        if (uiWidth != rInfo.Width ||
            uiHeight != rInfo.Height ||
            uiDepth != rInfo.Depth ||
            uiWidth > uiMaxSize ||
            uiHeight > uiMaxSize) {
            // Scaling the image is d3dx9's triangle filter, which the game's data never needs.
            Log("DDS of %ux%ux%u would need scaling, which is not supported", rInfo.Width, rInfo.Height, rInfo.Depth);
            return D3DXERR_INVALIDDATA;
        }
        const uint32_t uiFormat = ChooseFormat(apDevice, auiResourceType, rInfo.Format);
        if (uiFormat == D3DFMT_UNKNOWN) {
            Log("no device format for DDS format 0x%08X", rInfo.Format);
            return D3DERR_NOTAVAILABLE;
        }
        // D3DX_DEFAULT levels: the full chain, when the device mipmaps this resource type.
        const uint32_t uiMipCap = auiResourceType == D3DRTYPE_CUBETEXTURE
            ? D3DPTEXTURECAPS_MIPCUBEMAP
            : (auiResourceType == D3DRTYPE_VOLUMETEXTURE ? D3DPTEXTURECAPS_MIPVOLUMEMAP : D3DPTEXTURECAPS_MIPMAP);
        uint32_t uiLevels = 1;
        if (kCaps.TextureCaps & uiMipCap) {
            uint32_t uiLargest = uiWidth > uiHeight ? uiWidth : uiHeight;
            uiLargest = uiDepth > uiLargest ? uiDepth : uiLargest;
            while (uiLargest >> uiLevels) {
                ++uiLevels;
            }
        }
        if (auiResourceType == D3DRTYPE_TEXTURE) {
            hResult = apDevice->CreateTexture(
                uiWidth,
                uiHeight,
                uiLevels,
                0,
                uiFormat,
                D3DPOOL_MANAGED,
                reinterpret_cast<IDirect3DTexture9**>(appTexture),
                nullptr
            );
        }
        else if (auiResourceType == D3DRTYPE_CUBETEXTURE) {
            hResult = apDevice->CreateCubeTexture(
                uiWidth,
                uiLevels,
                0,
                uiFormat,
                D3DPOOL_MANAGED,
                reinterpret_cast<IDirect3DCubeTexture9**>(appTexture),
                nullptr
            );
        }
        else {
            hResult = apDevice->CreateVolumeTexture(
                uiWidth,
                uiHeight,
                uiDepth,
                uiLevels,
                0,
                uiFormat,
                D3DPOOL_MANAGED,
                reinterpret_cast<IDirect3DVolumeTexture9**>(appTexture),
                nullptr
            );
        }
        if (hResult < 0) {
            return hResult;
        }
        const FormatInfo& rSrcInfo = *FindFormat(rInfo.Format);
        const FormatInfo& rDstInfo = *FindFormat(uiFormat);
        const uint32_t uiFileLevels = rInfo.MipLevels < uiLevels ? rInfo.MipLevels : uiLevels;
        LevelLock kLock{*appTexture, auiResourceType};
        const uint8_t* pSrc = kFile.pixels;
        for (uint32_t uiFace = 0; uiFace < kFile.faces; ++uiFace) {
            Image kAbove;
            for (uint32_t uiLevel = 0; uiLevel < uiLevels; ++uiLevel) {
                const uint32_t uiW = Smaller(uiWidth, uiLevel);
                const uint32_t uiH = Smaller(uiHeight, uiLevel);
                const uint32_t uiD = Smaller(uiDepth, uiLevel);
                // The level's bytes in the texture's format, rows packed.
                std::vector<uint8_t> kConverted;
                const uint8_t* pLevel;
                if (uiLevel < uiFileLevels) {
                    pLevel = pSrc;
                    pSrc += LevelBytes(rSrcInfo, uiW, uiH, uiD);
                    if (uiFormat != rInfo.Format || uiFileLevels < uiLevels) {
                        kAbove = DecodeLevel(rSrcInfo, pLevel, uiW, uiH, uiD);
                    }
                    if (uiFormat != rInfo.Format) {
                        // The image filter's default, D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER.
                        kConverted = EncodeLevel(rDstInfo, kAbove, true);
                        pLevel = kConverted.data();
                    }
                }
                else {
                    // A level the file lacks: the box filter (the mip filter's default, no dither) of the level above
                    // as the texture holds it.
                    kConverted = EncodeLevel(rDstInfo, BoxDown(kAbove), false);
                    pLevel = kConverted.data();
                    if (uiLevel + 1 < uiLevels) {
                        kAbove = DecodeLevel(rDstInfo, pLevel, uiW, uiH, uiD);
                    }
                }
                if (uiLevel + 1 == uiFileLevels && uiFormat != rInfo.Format && uiFileLevels < uiLevels) {
                    kAbove = DecodeLevel(rDstInfo, pLevel, uiW, uiH, uiD);
                }
                uint8_t* pBits;
                uint32_t uiPitch;
                uint32_t uiSlicePitch;
                hResult = kLock.Lock(uiFace, uiLevel, pBits, uiPitch, uiSlicePitch);
                if (hResult < 0) {
                    (*appTexture)->Release();
                    *appTexture = nullptr;
                    return hResult;
                }
                const uint32_t uiRowBytes = RowBytes(rDstInfo, uiW);
                const uint32_t uiRows = RowCount(rDstInfo, uiH);
                for (uint32_t z = 0; z < uiD; ++z) {
                    for (uint32_t y = 0; y < uiRows; ++y) {
                        std::memcpy(
                            pBits + z * uiSlicePitch + y * uiPitch,
                            pLevel + (z * uiRows + y) * uiRowBytes,
                            uiRowBytes
                        );
                    }
                }
                kLock.Unlock(uiFace, uiLevel);
            }
            // The rest of the file's chain for this face, when the texture has fewer levels than the file.
            for (uint32_t uiLevel = uiFileLevels; uiLevel < rInfo.MipLevels; ++uiLevel) {
                pSrc += LevelBytes(
                    rSrcInfo,
                    Smaller(uiWidth, uiLevel),
                    Smaller(uiHeight, uiLevel),
                    Smaller(uiDepth, uiLevel)
                );
            }
        }
        return D3D_OK;
    }

    // A surface locked for reading: the surface itself, or (a render target's) copy in system memory.
    struct ReadLock {
        IDirect3DSurface9* locked = nullptr;
        IDirect3DSurface9* copy = nullptr;
        D3DLOCKED_RECT rect{};

        HRESULT Lock(IDirect3DSurface9* apSurface, const D3DSURFACE_DESC& arDesc) {
            if (apSurface->LockRect(&rect, nullptr, D3DLOCK_READONLY) >= 0) {
                locked = apSurface;
                return D3D_OK;
            }
            IDirect3DDevice9* pDevice = nullptr;
            HRESULT hResult = apSurface->GetDevice(&pDevice);
            if (hResult < 0) {
                return hResult;
            }
            hResult = pDevice->CreateOffscreenPlainSurface(
                arDesc.Width,
                arDesc.Height,
                arDesc.Format,
                D3DPOOL_SYSTEMMEM,
                &copy,
                nullptr
            );
            if (hResult >= 0) {
                hResult = pDevice->GetRenderTargetData(apSurface, copy);
            }
            pDevice->Release();
            if (hResult >= 0) {
                hResult = copy->LockRect(&rect, nullptr, D3DLOCK_READONLY);
            }
            if (hResult >= 0) {
                locked = copy;
            }
            return hResult;
        }

        ~ReadLock() {
            if (locked) {
                locked->UnlockRect();
            }
            if (copy) {
                copy->Release();
            }
        }
    };

    void Put32BE(std::vector<uint8_t>& arOut, uint32_t auiValue) {
        for (int32_t i = 3; i >= 0; --i) {
            arOut.push_back(static_cast<uint8_t>(auiValue >> (8 * i)));
        }
    }

    void PutChunk(std::vector<uint8_t>& arOut, const char* apType, const uint8_t* apData, uint32_t auiSize) {
        Put32BE(arOut, auiSize);
        const size_t uiStart = arOut.size();
        arOut.insert(arOut.end(), apType, apType + 4);
        arOut.insert(arOut.end(), apData, apData + auiSize);
        Put32BE(arOut, static_cast<uint32_t>(crc32(0, arOut.data() + uiStart, static_cast<uInt>(auiSize + 4))));
    }

} // namespace

extern "C" HRESULT WINAPI
D3DXGetImageInfoFromFileInMemory(const void* pSrcData, uint32_t SrcDataSize, D3DXIMAGE_INFO* pSrcInfo) {
    DdsFile kFile;
    const HRESULT hResult = ParseDds(pSrcData, SrcDataSize, kFile);
    if (hResult >= 0 && pSrcInfo) {
        *pSrcInfo = kFile.info;
    }
    return hResult;
}

extern "C" HRESULT WINAPI D3DXCreateTextureFromFileInMemory(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DTexture9** ppTexture
) {
    return CreateTextureFromDds(
        pDevice,
        pSrcData,
        SrcDataSize,
        D3DRTYPE_TEXTURE,
        reinterpret_cast<IDirect3DBaseTexture9**>(ppTexture)
    );
}

extern "C" HRESULT WINAPI D3DXCreateCubeTextureFromFileInMemory(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DCubeTexture9** ppCubeTexture
) {
    return CreateTextureFromDds(
        pDevice,
        pSrcData,
        SrcDataSize,
        D3DRTYPE_CUBETEXTURE,
        reinterpret_cast<IDirect3DBaseTexture9**>(ppCubeTexture)
    );
}

extern "C" HRESULT WINAPI D3DXCreateVolumeTextureFromFileInMemory(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    uint32_t SrcDataSize,
    IDirect3DVolumeTexture9** ppVolumeTexture
) {
    return CreateTextureFromDds(
        pDevice,
        pSrcData,
        SrcDataSize,
        D3DRTYPE_VOLUMETEXTURE,
        reinterpret_cast<IDirect3DBaseTexture9**>(ppVolumeTexture)
    );
}

// Whole surface to whole surface of the same size, no palettes and no color key: the game's one call.
extern "C" HRESULT WINAPI D3DXLoadSurfaceFromSurface(
    IDirect3DSurface9* pDestSurface,
    const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect,
    IDirect3DSurface9* pSrcSurface,
    const PALETTEENTRY* pSrcPalette,
    const RECT* pSrcRect,
    uint32_t Filter,
    uint32_t ColorKey
) {
    // D3DX_DEFAULT is D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER; at one size the filter only decides the dither.
    constexpr uint32_t D3DX_FILTER_DITHER = 0x80000;
    const bool bDither = Filter == D3DX_DEFAULT || (Filter & D3DX_FILTER_DITHER);
    if (!pDestSurface || !pSrcSurface) {
        return D3DERR_INVALIDCALL;
    }
    D3DSURFACE_DESC kSrcDesc;
    D3DSURFACE_DESC kDstDesc;
    if (pSrcSurface->GetDesc(&kSrcDesc) < 0 || pDestSurface->GetDesc(&kDstDesc) < 0) {
        return D3DERR_INVALIDCALL;
    }
    const FormatInfo* pSrcInfo = FindFormat(kSrcDesc.Format);
    const FormatInfo* pDstInfo = FindFormat(kDstDesc.Format);
    if (pDestPalette ||
        pDestRect ||
        pSrcPalette ||
        pSrcRect ||
        ColorKey ||
        kSrcDesc.Width != kDstDesc.Width ||
        kSrcDesc.Height != kDstDesc.Height ||
        !pSrcInfo ||
        !pDstInfo) {
        Log("D3DXLoadSurfaceFromSurface: only whole surfaces of one size in known formats are supported "
            "(0x%08X %ux%u to 0x%08X %ux%u)",
            kSrcDesc.Format,
            kSrcDesc.Width,
            kSrcDesc.Height,
            kDstDesc.Format,
            kDstDesc.Width,
            kDstDesc.Height);
        return E_NOTIMPL;
    }
    ReadLock kSrc;
    HRESULT hResult = kSrc.Lock(pSrcSurface, kSrcDesc);
    if (hResult < 0) {
        return hResult;
    }
    D3DLOCKED_RECT kDst;
    hResult = pDestSurface->LockRect(&kDst, nullptr, 0);
    if (hResult < 0) {
        return hResult;
    }
    const uint32_t uiRows = RowCount(*pDstInfo, kDstDesc.Height);
    const uint32_t uiRowBytes = RowBytes(*pDstInfo, kDstDesc.Width);
    uint8_t* pDstBits = static_cast<uint8_t*>(kDst.pBits);
    if (kSrcDesc.Format == kDstDesc.Format) {
        const uint8_t* pSrcBits = static_cast<const uint8_t*>(kSrc.rect.pBits);
        for (uint32_t y = 0; y < uiRows; ++y) {
            std::memcpy(pDstBits + y * kDst.Pitch, pSrcBits + y * kSrc.rect.Pitch, uiRowBytes);
        }
    }
    else {
        Image kImage;
        kImage.width = kSrcDesc.Width;
        kImage.height = kSrcDesc.Height;
        kImage.pixels.resize(static_cast<size_t>(kImage.width) * kImage.height);
        DecodeSlice(
            *pSrcInfo,
            static_cast<const uint8_t*>(kSrc.rect.pBits),
            static_cast<uint32_t>(kSrc.rect.Pitch),
            kImage,
            0
        );
        EncodeSlice(*pDstInfo, kImage, 0, pDstBits, static_cast<uint32_t>(kDst.Pitch), bDither);
    }
    pDestSurface->UnlockRect();
    return D3D_OK;
}

// Level 0 of a 2D texture as an 8-bit RGB PNG, RGBA when the format has alpha. Other file formats and texture types
// are not supported (the game saves only PNG, MapMenu's local maps).
extern "C" HRESULT WINAPI D3DXSaveTextureToFileA(
    const char* pDestFile,
    uint32_t DestFormat,
    IDirect3DBaseTexture9* pSrcTexture,
    const PALETTEENTRY* pSrcPalette
) {
    (void)pSrcPalette;
    if (!pDestFile || !pSrcTexture) {
        return D3DERR_INVALIDCALL;
    }
    if (DestFormat != D3DXIFF_PNG || pSrcTexture->GetType() != D3DRTYPE_TEXTURE) {
        Log("D3DXSaveTextureToFileA: only a 2D texture to PNG is supported (%s)", pDestFile);
        return E_NOTIMPL;
    }
    IDirect3DSurface9* pSurface = nullptr;
    HRESULT hResult = static_cast<IDirect3DTexture9*>(pSrcTexture)->GetSurfaceLevel(0, &pSurface);
    if (hResult < 0) {
        return hResult;
    }
    D3DSURFACE_DESC kDesc;
    pSurface->GetDesc(&kDesc);
    const FormatInfo* pInfo = FindFormat(kDesc.Format);
    if (!pInfo) {
        pSurface->Release();
        Log("D3DXSaveTextureToFileA: format 0x%08X is not supported (%s)", kDesc.Format, pDestFile);
        return E_NOTIMPL;
    }
    Image kImage;
    kImage.width = kDesc.Width;
    kImage.height = kDesc.Height;
    kImage.pixels.resize(static_cast<size_t>(kImage.width) * kImage.height);
    {
        ReadLock kLock;
        hResult = kLock.Lock(pSurface, kDesc);
        if (hResult >= 0) {
            DecodeSlice(
                *pInfo,
                static_cast<const uint8_t*>(kLock.rect.pBits),
                static_cast<uint32_t>(kLock.rect.Pitch),
                kImage,
                0
            );
        }
    }
    pSurface->Release();
    if (hResult < 0) {
        return hResult;
    }
    const bool bAlpha = pInfo->aMask || pInfo->block;
    const uint32_t uiChannels = bAlpha ? 4 : 3;
    // The rows, each behind PNG's filter byte 0 (none).
    std::vector<uint8_t> kRaw;
    kRaw.reserve((static_cast<size_t>(kImage.width) * uiChannels + 1) * kImage.height);
    auto Byte = [](float afValue) {
        afValue = afValue < 0.0f ? 0.0f : (afValue > 1.0f ? 1.0f : afValue);
        return static_cast<uint8_t>(afValue * 255.0f + 0.5f);
    };
    for (uint32_t y = 0; y < kImage.height; ++y) {
        kRaw.push_back(0);
        for (uint32_t x = 0; x < kImage.width; ++x) {
            const Color& rColor = kImage.at(x, y);
            kRaw.push_back(Byte(rColor.r));
            kRaw.push_back(Byte(rColor.g));
            kRaw.push_back(Byte(rColor.b));
            if (bAlpha) {
                kRaw.push_back(Byte(rColor.a));
            }
        }
    }
    uLongf ulPacked = compressBound(static_cast<uLong>(kRaw.size()));
    std::vector<uint8_t> kPacked(ulPacked);
    if (compress2(kPacked.data(), &ulPacked, kRaw.data(), static_cast<uLong>(kRaw.size()), 6) != Z_OK) {
        return E_OUTOFMEMORY;
    }
    std::vector<uint8_t> kFile = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<uint8_t> kHeader;
    Put32BE(kHeader, kImage.width);
    Put32BE(kHeader, kImage.height);
    // 8 bits per channel, color type 6 (RGBA) or 2 (RGB), deflate, adaptive filtering, no interlace.
    const uint8_t aucRest[] = {8, static_cast<uint8_t>(bAlpha ? 6 : 2), 0, 0, 0};
    kHeader.insert(kHeader.end(), aucRest, aucRest + 5);
    PutChunk(kFile, "IHDR", kHeader.data(), static_cast<uint32_t>(kHeader.size()));
    PutChunk(kFile, "IDAT", kPacked.data(), static_cast<uint32_t>(ulPacked));
    PutChunk(kFile, "IEND", nullptr, 0);
    FILE* pFile = std::fopen(pDestFile, "wb");
    if (!pFile) {
        Log("D3DXSaveTextureToFileA: cannot open %s", pDestFile);
        return D3DERR_INVALIDCALL;
    }
    const bool bWritten = std::fwrite(kFile.data(), 1, kFile.size(), pFile) == kFile.size();
    std::fclose(pFile);
    return bWritten ? D3D_OK : D3DERR_INVALIDCALL;
}
