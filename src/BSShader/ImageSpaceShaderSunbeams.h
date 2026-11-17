#pragma once

#include "BSShader/ImageSpaceShader.h"
#include "Reconstruction.hpp"

// The image-space shader that draws the sun's beams through the stencil (the Xbox PDB puts it in
// BSShader\ImageSpaceManager.cpp).
class ImageSpaceShaderSunbeams : public ImageSpaceShader {
public:
	ImageSpaceShaderSunbeams();
	~ImageSpaceShaderSunbeams() override;

	// Loads the sunbeams shaders.
	void ReloadShaders() override;
	void SetState() override;
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShaderSunbeams_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShaderSunbeams_vftable{};
