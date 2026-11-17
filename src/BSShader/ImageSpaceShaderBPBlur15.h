#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBPBlur15 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur15();
	~ImageSpaceShaderBPBlur15() override;

	void ReloadShaders() override;
};
