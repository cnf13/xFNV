#pragma once

#include "BSShader/ImageSpaceShader.h"

// The fifteen-tap blur shader. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderBlur15 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur15();
	~ImageSpaceShaderBlur15() override;

	void ReloadShaders() override;
};
