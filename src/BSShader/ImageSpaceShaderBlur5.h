#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur5 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur5();
	~ImageSpaceShaderBlur5() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B0D1C.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur5_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur5_vftable_ImageSpaceEffect{};
