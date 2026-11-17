#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur5 : public ImageSpaceShader {
public:
	ImageSpaceShaderBlur5();
	~ImageSpaceShaderBlur5() override;

	void ReloadShaders() override;
};
