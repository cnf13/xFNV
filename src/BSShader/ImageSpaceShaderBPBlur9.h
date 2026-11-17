#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderBPBlur9 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur9();
	~ImageSpaceShaderBPBlur9() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B1AE8.
inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur9_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur9_vftable_ImageSpaceEffect{};
