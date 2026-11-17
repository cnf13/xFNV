#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur();
	~ImageSpaceShaderBlur() override;

	void ReloadShaders() override;
};
