#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderDoubleVision : public ImageSpaceShader {
public:
	ImageSpaceShaderDoubleVision();
	~ImageSpaceShaderDoubleVision() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDoubleVision_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B33CC; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderDoubleVision_vftable_ImageSpaceEffect{};
