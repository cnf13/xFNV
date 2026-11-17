#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Ni2DBuffer.h"

class NiDX9Renderer;
class NiRenderedTexture;

// The DX9 renderer's data of a rendered texture (a render target). Only some of its members are declared.
class NiDX9RenderedTextureData : public NiDX9TextureData {
public:
	// The fourth argument is unused (0).
	NiDX9RenderedTextureData(NiTexture* pkTexture, NiDX9Renderer* pkRenderer, uint32_t auiUnused);
	~NiDX9RenderedTextureData() override;

	NiDX9RenderedTextureData* GetAsRenderedTexture() const override {
		return const_cast<NiDX9RenderedTextureData*>(this);
	}

	// Creates the render target surface and the texture from the texture's size and preferences; returns the pixel
	// format it used, null when the device fails.
	const NiPixelFormat* CreateSurf(const NiRenderedTexture* pkTexture, Ni2DBuffer::MultiSamplePreference eMSAAPref);

	// Creates the render target of apTexture; aeMultiSample is a Ni2DBuffer::MultiSamplePreference.
	static NiDX9RenderedTextureData* Create(NiRenderedTexture* apTexture, NiDX9Renderer* apRenderer,
		uint32_t aeMultiSample);

	// The render target surface CreateSurf makes beside the texture.
	IDirect3DSurface9* m_pkRenderTarget;
};
