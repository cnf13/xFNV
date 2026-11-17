#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderDepthOfField : public ImageSpaceShader {
public:
	ImageSpaceShaderDepthOfField();
	~ImageSpaceShaderDepthOfField() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDepthOfField_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B2BEC; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderDepthOfField_vftable_ImageSpaceEffect{};
