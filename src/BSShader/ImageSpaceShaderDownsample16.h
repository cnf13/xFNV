#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDownsample16 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample16();
	~ImageSpaceShaderDownsample16() override;

	void ReloadShaders() override;
};
