#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur5 : public ImageSpaceShader {
public:
	ImageSpaceShaderBrightPassBlur5();
	~ImageSpaceShaderBrightPassBlur5() override;

	void ReloadShaders() override;
};
