#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderScanlinesBlend_ : public ImageSpaceShader {
public:
	ImageSpaceShaderScanlinesBlend_();
	~ImageSpaceShaderScanlinesBlend_() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderScanlinesBlend__vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; ImageSpaceShader.h overlays it as rdata_10B4C48.
inline constexpr fnvd::Vtable ImageSpaceShaderScanlinesBlend__vftable_ImageSpaceEffect{};
