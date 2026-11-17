#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderBPBlur13 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur13();
	~ImageSpaceShaderBPBlur13() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B1DFC.
inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur13_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur13_vftable_ImageSpaceEffect{};
