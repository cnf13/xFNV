#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDownsample9 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample9();
	~ImageSpaceShaderDownsample9() override;

	void ReloadShaders() override;
};
