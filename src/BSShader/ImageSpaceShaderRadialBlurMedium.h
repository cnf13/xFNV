#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderRadialBlurMedium : public ImageSpaceShader {
public:
	ImageSpaceShaderRadialBlurMedium();
	~ImageSpaceShaderRadialBlurMedium() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderRadialBlurMedium_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B2F30; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderRadialBlurMedium_vftable_ImageSpaceEffect{};
