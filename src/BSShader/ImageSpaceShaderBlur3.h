#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur3 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur3();
	~ImageSpaceShaderBlur3() override;

	void ReloadShaders() override;
};
