#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderMotionBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderMotionBlur();
	~ImageSpaceShaderMotionBlur() override;

	void ReloadShaders() override;
};
