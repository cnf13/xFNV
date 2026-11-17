#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderNoiseScrollAndBlend : public ImageSpaceShader {
public:
	ImageSpaceShaderNoiseScrollAndBlend();
	~ImageSpaceShaderNoiseScrollAndBlend() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderNoiseScrollAndBlend_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable ImageSpaceShaderNoiseScrollAndBlend_vftable_ImageSpaceEffect{};
