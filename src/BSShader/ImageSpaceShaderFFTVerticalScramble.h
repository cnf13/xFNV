#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderFFTVerticalScramble : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTVerticalScramble();
	~ImageSpaceShaderFFTVerticalScramble() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B7258.
inline constexpr fnvd::Vtable ImageSpaceShaderFFTVerticalScramble_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderFFTVerticalScramble_vftable_ImageSpaceEffect{};
