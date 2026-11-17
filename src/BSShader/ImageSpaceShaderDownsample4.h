#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDownsample4 : public ImageSpaceShader {
public:
	ImageSpaceShaderDownsample4();
	~ImageSpaceShaderDownsample4() override;

	void ReloadShaders() override;
};
