#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur13 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur13();
	~ImageSpaceShaderBlur13() override;

	void ReloadShaders() override;
};
