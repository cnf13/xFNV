#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderRadialBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderRadialBlur();
	~ImageSpaceShaderRadialBlur() override;

	void ReloadShaders() override;
};
