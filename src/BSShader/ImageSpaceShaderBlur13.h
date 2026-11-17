#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur13 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur13();
	~ImageSpaceShaderBlur13() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B1320.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur13_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur13_vftable_ImageSpaceEffect{};
