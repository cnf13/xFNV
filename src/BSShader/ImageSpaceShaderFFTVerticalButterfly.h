#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderFFTVerticalButterfly : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTVerticalButterfly();
	~ImageSpaceShaderFFTVerticalButterfly() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B6F14.
inline constexpr fnvd::Vtable ImageSpaceShaderFFTVerticalButterfly_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderFFTVerticalButterfly_vftable_ImageSpaceEffect{};
