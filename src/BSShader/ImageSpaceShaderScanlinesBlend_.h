#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderScanlinesBlend_ : public ImageSpaceShader {
public:
	ImageSpaceShaderScanlinesBlend_();
	~ImageSpaceShaderScanlinesBlend_() override;

	void ReloadShaders() override;
};
