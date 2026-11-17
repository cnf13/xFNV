#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderHDRBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderHDRBlur();
	~ImageSpaceShaderHDRBlur() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlur_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B3AD0.
inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlur_vftable_ImageSpaceEffect{};
