#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBPBlur9 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur9();
	~ImageSpaceShaderBPBlur9() override;

	void ReloadShaders() override;
};
