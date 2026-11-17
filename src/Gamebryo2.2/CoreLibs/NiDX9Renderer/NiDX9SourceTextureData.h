#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiDX9PersistentSrcTextureRendererData;
class NiDX9Renderer;
class NiFile;
class NiSourceTexture;

// The DX9 renderer's data of a source texture. Not in the Xbox pdb.
// Only the members called so far; the rest is not reconstructed yet.
class NiDX9SourceTextureData : public NiDX9TextureData {
public:
	NiDX9SourceTextureData(NiSourceTexture* texture, NiDX9Renderer* a3);

	NiDX9SourceTextureData* GetAsSourceTexture() const override { return const_cast<NiDX9SourceTextureData*>(this); }

	// slot 44.
	bool InitializeFromD3DTexture(IDirect3DBaseTexture9* apTexture) override;
	// slot 45, 0xE689D0
	virtual void Update();
	// slot 46. The palette of the source pixel data, m_spPalette.
	virtual NiPalette* GetPalette();

	// Creates the texture of format aeD3DFmt with the persistent renderer data's level count, dropping the top
	// ms_uiSkipLevels levels; false and no texture when Direct3D refuses it. IDA name; not in the PDB. thiscall,
	// ret 8, 0xE682B0.
	bool CreateSurfFromRendererData(const NiDX9PersistentSrcTextureRendererData* apkData, D3DFORMAT aeD3DFmt);
	// Copies every mipmap level above m_uiLevelsSkipped of apkPixels into the texture's surfaces. IDA name; not in
	// the PDB. 0xE68870.
	void CopyDataToSurface(NiPixelData* apkPixels);
	// Copies every mipmap level above m_uiLevelsSkipped of the persistent renderer data into the texture's surfaces.
	void CopyRendererDataToSurface(const NiDX9PersistentSrcTextureRendererData* apkData);
	bool CreateSurf(const NiPixelData* apkPixels);
	// Inferred type: a1: char* from 0xe68fb7 (NiFixedString::m_kHandle (parameter 0 of
	// NiDX9SourceTextureData::Create)); a2: NiDX9Renderer* from 0xe68fb7 (parameter 1 of
	// NiDX9SourceTextureData::Create), its members used at 0xe68ab6
	bool LoadTextureFile(const char* a1, NiDX9Renderer* a2, NiTexture::FormatPrefs* a3, NiFile* a4);

	// cdecl, 0xE68EF0. Creates the renderer data of apTexture.
	static NiDX9SourceTextureData* Create(NiSourceTexture* apTexture, NiDX9Renderer* apRenderer);
	// Copies mipmap level auiLevel of face auiFace of the persistent renderer data into the locked surface
	// apkSurface. IDA name; not in the PDB. cdecl, 0xE683D0.
	// Inferred type: apkData: NiDX9PersistentSrcTextureRendererData* from 0xe8aba4 (parameter 0 of
	// NiDX9SourceCubeMapData::CopyRendererDataToCubeMapFace), its members used at 0xe68434
	static void CopyRendererDataToSurfaceLevel(const NiDX9PersistentSrcTextureRendererData* apkData, uint32_t auiLevel,
		IDirect3DSurface9* apkSurface, uint32_t auiFace);
	// Copies mipmap level auiLevel of face auiFace of apkPixels into the locked surface apkMipLevel, one row at a
	// time when the surface's pitch differs from the level's row size. IDA name; not in the PDB. cdecl, 0xE68150.
	// Inferred type: apkPixels: NiPixelData* from 0xe688d7 (parameter 0 of NiDX9SourceTextureData::CopyDataToSurface),
	// its members used at 0xe681b4
	static void CopyDataToSurfaceLevel(NiPixelData* apkPixels, uint32_t auiLevel, IDirect3DSurface9* apkMipLevel,
		uint32_t auiFace);

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
	// IDA names; not in the PDB. The iTexMipMapSkip:Display and iTexMipMapMinimum:Display settings, copied here by
	// Renderer::Init.
	static inline uint32_t uiMipMapSkip;
	static inline uint32_t uiMipMapMinimum;
};
inline constexpr fnvd::Vtable NiDX9SourceTextureData_vftable{};
