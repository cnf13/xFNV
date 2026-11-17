#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur11 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur11();
	~ImageSpaceShaderBlur11() override;

	void ReloadShaders() override;
};
