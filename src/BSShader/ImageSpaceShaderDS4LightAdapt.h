#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderDS4LightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderDS4LightAdapt();
	~ImageSpaceShaderDS4LightAdapt() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDS4LightAdapt_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B45DC.
inline constexpr fnvd::Vtable ImageSpaceShaderDS4LightAdapt_vftable_ImageSpaceEffect{};
