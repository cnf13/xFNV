#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlend : public ImageSpaceShader {
public:
	ImageSpaceShaderBlend();
	~ImageSpaceShaderBlend() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B083C.
inline constexpr fnvd::Vtable ImageSpaceShaderBlend_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlend_vftable_ImageSpaceEffect{};
