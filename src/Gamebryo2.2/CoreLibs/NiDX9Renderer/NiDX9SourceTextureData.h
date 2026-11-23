#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiDX9PersistentSrcTextureRendererData;
class NiDX9Renderer;
class NiFile;
class NiSourceTexture;

// The DX9 renderer's data of a source texture. Only some of its members are declared.
class NiDX9SourceTextureData : public NiDX9TextureData {
public:
    NiDX9SourceTextureData(NiSourceTexture* texture, NiDX9Renderer* pkRenderer);

    NiDX9SourceTextureData* GetAsSourceTexture() const override {
        return const_cast<NiDX9SourceTextureData*>(this);
    }

    bool InitializeFromD3DTexture(IDirect3DBaseTexture9* apTexture) override;
    virtual void Update();
    // The palette of the source pixel data, m_spPalette.
    virtual NiPalette* GetPalette();

    // Creates the texture of format aeD3DFmt with the persistent renderer data's level count, dropping the top
    // ms_uiSkipLevels levels; false and no texture when Direct3D refuses it.
    bool CreateSurfFromRendererData(const NiDX9PersistentSrcTextureRendererData* apkData, D3DFORMAT aeD3DFmt);
    // Copies every mipmap level above m_uiLevelsSkipped of apkPixels into the texture's surfaces.
    void CopyDataToSurface(NiPixelData* apkPixels);
    // Copies every mipmap level above m_uiLevelsSkipped of the persistent renderer data into the texture's surfaces.
    void CopyRendererDataToSurface(const NiDX9PersistentSrcTextureRendererData* apkData);
    bool CreateSurf(const NiPixelData* apkPixels);
    bool LoadTextureFile(
        const char* pcFilename,
        NiDX9Renderer* pkRenderer,
        NiTexture::FormatPrefs* pkPrefs,
        NiFile* pkSourceFile
    );

    // Creates the renderer data of apTexture.
    static NiDX9SourceTextureData* Create(NiSourceTexture* apTexture, NiDX9Renderer* apRenderer);
    // Copies mipmap level auiLevel of face auiFace of the persistent renderer data into the locked surface apkSurface.
    static void CopyRendererDataToSurfaceLevel(
        const NiDX9PersistentSrcTextureRendererData* apkData,
        uint32_t auiLevel,
        IDirect3DSurface9* apkSurface,
        uint32_t auiFace
    );
    // Copies mipmap level auiLevel of face auiFace of apkPixels into the locked surface apkMipLevel, one row at a time
    // when the surface's pitch differs from the level's row size.
    static void
    CopyDataToSurfaceLevel(NiPixelData* apkPixels, uint32_t auiLevel, IDirect3DSurface9* apkMipLevel, uint32_t auiFace);

    bool m_bReplacementData;
    bool m_bMipmap;
    uint32_t m_uiFormattedSize;
    NiPointer<NiPalette> m_spPalette;
    uint32_t m_uiLevelsSkipped;
    uint32_t m_uiSourceRevID;
    uint32_t m_uiPalRevID;

    // The number of top mip levels source textures skip when their surfaces are created (CreateSurf); set through
    // NiDX9Renderer::SetMipmapSkipLevel.
    static inline uint32_t ms_uiSkipLevels;
    // The iTexMipMapSkip:Display and iTexMipMapMinimum:Display settings, copied here by Renderer::Init.
    static inline uint32_t uiMipMapSkip;
    static inline uint32_t uiMipMapMinimum;
};
