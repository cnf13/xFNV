#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBPBlur13 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur13();
	~ImageSpaceShaderBPBlur13() override;

	void ReloadShaders() override;
};
