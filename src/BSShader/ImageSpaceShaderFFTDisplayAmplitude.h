#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderFFTDisplayAmplitude : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTDisplayAmplitude();
	~ImageSpaceShaderFFTDisplayAmplitude() override;

	void ReloadShaders() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B7A2C.
inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayAmplitude_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderFFTDisplayAmplitude_vftable_ImageSpaceEffect{};
