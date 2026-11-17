#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderRadialBlurHigh : public ImageSpaceShader {
public:
	ImageSpaceShaderRadialBlurHigh();
	~ImageSpaceShaderRadialBlurHigh() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderRadialBlurHigh_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B30C0; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderRadialBlurHigh_vftable_ImageSpaceEffect{};
