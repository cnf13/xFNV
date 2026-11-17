#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderAlphaMult : public ImageSpaceShader {
public:
	ImageSpaceShaderAlphaMult();
	~ImageSpaceShaderAlphaMult() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B06A0.
inline constexpr fnvd::Vtable ImageSpaceShaderAlphaMult_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderAlphaMult_vftable_ImageSpaceEffect{};
