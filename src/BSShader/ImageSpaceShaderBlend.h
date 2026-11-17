#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlend : public ImageSpaceShader {
public:
	ImageSpaceShaderBlend();
	~ImageSpaceShaderBlend() override;

	void ReloadShaders() override;
};
