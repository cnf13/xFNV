#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderCinematic : public ImageSpaceShader {
public:
	ImageSpaceShaderCinematic();
	~ImageSpaceShaderCinematic() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderCinematic_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B211C; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderCinematic_vftable_ImageSpaceEffect{};
