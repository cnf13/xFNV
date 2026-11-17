#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderNULL : public ImageSpaceShader {
public:
	ImageSpaceShaderNULL();
	~ImageSpaceShaderNULL() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderNULL_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B2784; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderNULL_vftable_ImageSpaceEffect{};
