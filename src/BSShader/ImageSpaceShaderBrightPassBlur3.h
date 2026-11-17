#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur3 : public ImageSpaceShader {
public:
	ImageSpaceShaderBrightPassBlur3();
	~ImageSpaceShaderBrightPassBlur3() override;

	void ReloadShaders() override;
};
