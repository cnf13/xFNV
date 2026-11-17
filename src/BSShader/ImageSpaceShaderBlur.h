#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur();
	~ImageSpaceShaderBlur() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B09DC.
inline constexpr fnvd::Vtable ImageSpaceShaderBlur_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBlur_vftable_ImageSpaceEffect{};
