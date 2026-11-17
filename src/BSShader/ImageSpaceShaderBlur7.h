#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur7 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur7();
	~ImageSpaceShaderBlur7() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B0E9C.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur7_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur7_vftable_ImageSpaceEffect{};
