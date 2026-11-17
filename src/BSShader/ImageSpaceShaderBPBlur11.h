#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBPBlur11 : public ImageSpaceShader {
public:
	ImageSpaceShaderBPBlur11();
	~ImageSpaceShaderBPBlur11() override;

	void ReloadShaders() override;
};
