#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderScanlinesBlend : public ImageSpaceShader {
public:
	ImageSpaceShaderScanlinesBlend();
	~ImageSpaceShaderScanlinesBlend() override;

	void ReloadShaders() override;
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShaderScanlinesBlend_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShaderScanlinesBlend_vftable{};
