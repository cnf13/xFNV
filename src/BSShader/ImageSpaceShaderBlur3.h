#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur3 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur3();
	~ImageSpaceShaderBlur3() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B0B78.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur3_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur3_vftable_ImageSpaceEffect{};
