#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur11 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur11();
	~ImageSpaceShaderBlur11() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B119C.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur11_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur11_vftable_ImageSpaceEffect{};
