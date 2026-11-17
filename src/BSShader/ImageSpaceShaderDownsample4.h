#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderDownsample4 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample4();
	~ImageSpaceShaderDownsample4() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDownsample4_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; IDA calls the data rdata_10B4114.
inline constexpr fnvd::Vtable ImageSpaceShaderDownsample4_vftable_ImageSpaceEffect{};
