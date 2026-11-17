#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderMotionBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderMotionBlur();
	~ImageSpaceShaderMotionBlur() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderMotionBlur_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable ImageSpaceShaderMotionBlur_vftable_ImageSpaceEffect{};
