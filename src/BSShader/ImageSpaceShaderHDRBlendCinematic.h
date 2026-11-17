#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderHDRBlendCinematic : public ImageSpaceShader {
public:
	ImageSpaceShaderHDRBlendCinematic();
	~ImageSpaceShaderHDRBlendCinematic() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlendCinematic_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B3788.
inline constexpr fnvd::Vtable ImageSpaceShaderHDRBlendCinematic_vftable_ImageSpaceEffect{};
