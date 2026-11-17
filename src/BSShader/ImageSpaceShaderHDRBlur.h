#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlur : public ImageSpaceShader {
public:
	ImageSpaceShaderHDRBlur();
	~ImageSpaceShaderHDRBlur() override;

	void ReloadShaders() override;
};
