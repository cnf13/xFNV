#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderFFTHorizontalScramble : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTHorizontalScramble();
	~ImageSpaceShaderFFTHorizontalScramble() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};

// The class's primary vtable and its secondary ImageSpaceEffect vtable (stored at offset 0x6C). IDA names the
// secondary table rdata_10B70B8.
inline constexpr fnvd::Vtable ImageSpaceShaderFFTHorizontalScramble_vftable{};
inline constexpr fnvd::Vtable ImageSpaceShaderFFTHorizontalScramble_vftable_ImageSpaceEffect{};
