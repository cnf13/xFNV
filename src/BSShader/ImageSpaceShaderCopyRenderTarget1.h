#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderCopyRenderTarget1 : public ImageSpaceShader {
public:
	ImageSpaceShaderCopyRenderTarget1();
	~ImageSpaceShaderCopyRenderTarget1() override;

	void ReloadShaders() override;
	void SetState() override;
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShaderCopyRenderTarget1_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShaderCopyRenderTarget1_vftable{};
