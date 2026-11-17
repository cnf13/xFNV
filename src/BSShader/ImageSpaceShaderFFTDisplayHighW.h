#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderFFTDisplayHighW : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTDisplayHighW();
	~ImageSpaceShaderFFTDisplayHighW() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B7898.
inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayHighW_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayHighW_vftable_ImageSpaceEffect{};
