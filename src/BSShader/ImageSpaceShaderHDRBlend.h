#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderHDRBlend : public ImageSpaceShader {
public:
	ImageSpaceShaderHDRBlend();
	~ImageSpaceShaderHDRBlend() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlend_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B3574.
inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlend_vftable_ImageSpaceEffect{};
