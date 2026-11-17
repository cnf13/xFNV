#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader. The Xbox PDB has no class of this name; the name is the exe's vtable symbol.
class ImageSpaceShaderBPBlur15 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur15();
	~ImageSpaceShaderBPBlur15() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur15_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B1F8C; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderBPBlur15_vftable_ImageSpaceEffect{};
