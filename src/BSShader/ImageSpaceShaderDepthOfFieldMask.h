#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderDepthOfFieldMask : public ImageSpaceShader {
public:
	ImageSpaceShaderDepthOfFieldMask();
	~ImageSpaceShaderDepthOfFieldMask() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderDepthOfFieldMask_vftable{};
// The ImageSpaceEffect subobject's vtable (offset 0x6C); IDA name rdata_10B3244; not in the PDB.
inline constexpr fnvd::Vtable ImageSpaceShaderDepthOfFieldMask_vftable_ImageSpaceEffect{};
