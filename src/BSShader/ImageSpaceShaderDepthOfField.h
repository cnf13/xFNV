#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDepthOfField : public ImageSpaceShader {
public:
	ImageSpaceShaderDepthOfField();
	~ImageSpaceShaderDepthOfField() override;

	void ReloadShaders() override;
};
