#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderRestoreEDRAM : public ImageSpaceShader {
public:
	ImageSpaceShaderRestoreEDRAM();
	~ImageSpaceShaderRestoreEDRAM() override;

	void ReloadShaders() override {}
};

inline constexpr fnvd::Vtable ImageSpaceShaderRestoreEDRAM_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B28FC; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderRestoreEDRAM_vftable_ImageSpaceEffect{};
