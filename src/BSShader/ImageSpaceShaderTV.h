#pragma once

#include "BSShader/ImageSpaceShader.h"

// The television post-processing shader. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderTV : public ImageSpaceShader {
public:
	ImageSpaceShaderTV();

	void ReloadShaders() override;
};
