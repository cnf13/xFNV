#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// An image-space shader (the Xbox PDB puts it in BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderMap : public ImageSpaceShader {
public:
	ImageSpaceShaderMap();
	~ImageSpaceShaderMap() override;

	void ReloadShaders() override;
};

inline constexpr fnvd::Vtable ImageSpaceShaderMap_vftable{};
// The ImageSpaceEffect subobject's vtable, stored at +0x6C.
inline constexpr fnvd::Vtable ImageSpaceShaderMap_vftable_ImageSpaceEffect{};
