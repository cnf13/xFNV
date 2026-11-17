#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBrightPassBlur5 : public ImageSpaceShader {
public:
	ImageSpaceShaderBrightPassBlur5();
	~ImageSpaceShaderBrightPassBlur5() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B17D8.
inline constexpr fnvd::Vtable ImageSpaceShaderBrightPassBlur5_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBrightPassBlur5_vftable_ImageSpaceEffect{};
