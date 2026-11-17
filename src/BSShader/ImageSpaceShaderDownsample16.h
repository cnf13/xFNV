#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderDownsample16 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample16();
	~ImageSpaceShaderDownsample16() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDownsample16_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B3DF4.
inline constexpr fnvd::Vtable ImageSpaceShaderDownsample16_vftable_ImageSpaceEffect{};
