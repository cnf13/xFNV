#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

#include <stdint.h>

class NiDX9DynamicTextureData;
class NiDX9RenderedTextureData;
class NiDX9Renderer;
class NiDX9SourceTextureData;

// The DX9 renderer's data of a texture: the D3D texture and its level count.
class NiDX9TextureData : public NiTexture::RendererData {
public:
	// The constructor: the RendererData construction, the texture and the renderer.
	NiDX9TextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer);
	~NiDX9TextureData() override;

	virtual uint32_t GetLevels() const { return m_uiLevels; }
	virtual IDirect3DBaseTexture9* GetD3DTexture() const { return m_pkD3DTexture; }
	virtual void SetD3DTexture(IDirect3DBaseTexture9* apTexture) { m_pkD3DTexture = apTexture; }
	virtual NiDX9RenderedTextureData* GetAsRenderedTexture() const { return nullptr; }
	virtual NiDX9SourceTextureData* GetAsSourceTexture() const { return nullptr; }
	virtual NiDX9DynamicTextureData* GetAsDynamicTexture() const { return nullptr; }
	virtual bool InitializeFromD3DTexture(IDirect3DBaseTexture9* apTexture);

	// The first of the renderer's formats that akSrcFmt converts to, tried in an order chosen by the source format
	// and the preferences; null for a tiled source or when none converts.
	NiPixelFormat* FindMatchingPixelFormat(const NiPixelFormat& akSrcFmt, const NiTexture::FormatPrefs& akPrefs,
		NiPixelFormat** appkDestFmts);

	// The first of the renderer's formats (appkFormats, by NiDX9Renderer::TexFormat) that the preferences accept, in
	// the preferences' order of preference; null when none is available.
	static NiPixelFormat* FindClosestPixelFormat(const NiTexture::FormatPrefs* apPrefs, NiPixelFormat** appkFormats);
	// Deletes the renderer data of every texture in the texture list.
	static void ClearTextureData();

	NiDX9Renderer* m_pkRenderer;
	IDirect3DBaseTexture9* m_pkD3DTexture;
	uint32_t m_uiLevels;
};
