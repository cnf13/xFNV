#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderDS16LightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderDS16LightAdapt();
	~ImageSpaceShaderDS16LightAdapt() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDS16LightAdapt_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B4914.
inline constexpr fnvd::Vtable ImageSpaceShaderDS16LightAdapt_vftable_ImageSpaceEffect{};
