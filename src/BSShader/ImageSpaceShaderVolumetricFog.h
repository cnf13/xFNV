#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderVolumetricFog : public ImageSpaceShader {
public:
	ImageSpaceShaderVolumetricFog();
	~ImageSpaceShaderVolumetricFog() override;

	void ReloadShaders() override;
	void SetState() override;
	// The ImageSpaceEffect IsActive slot, which the exe reaches only through the shader's secondary vtable at +0x6C
	// (0x10B04F4): active while texture slot 2 holds a texture.
	bool IsActive() override;

	// Inferred type: return: BSRenderedTexture* from 0xb63cf8 (result of ImageSpaceTexture::GetRenderedTexture)
	BSRenderedTexture* GetTexture();
	// IDA name (as ImageSpaceManager's); not in the PDB. Returns the rendered texture of texture slot 2 to the
	// texture manager. Its callers pass this shader (its destructor; RenderVolumetricFog: effect list entry 42 less
	// the 0x6C ImageSpaceEffect base), and it reads pTextures.m_pBase at +0x98 (thiscall, ret 0, 0xB63D10).
	void ReturnVolumetricFogTexture();
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShaderVolumetricFog_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShaderVolumetricFog_vftable{};
