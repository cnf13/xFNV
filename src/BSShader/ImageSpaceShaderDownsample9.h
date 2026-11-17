#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderDownsample9 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample9();
	~ImageSpaceShaderDownsample9() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDownsample9_vftable{};
// The vtable of the ImageSpaceEffect subobject at offset 0x6C; IDA calls the data rdata_10B3F88.
inline constexpr fnvd::Vtable ImageSpaceShaderDownsample9_vftable_ImageSpaceEffect{};
