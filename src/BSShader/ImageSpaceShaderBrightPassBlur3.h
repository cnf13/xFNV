#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBrightPassBlur3 : public ImageSpaceShader {
public:
	ImageSpaceShaderBrightPassBlur3();
	~ImageSpaceShaderBrightPassBlur3() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B1620.
inline constexpr fnvd::Vtable ImageSpaceShaderBrightPassBlur3_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBrightPassBlur3_vftable_ImageSpaceEffect{};
