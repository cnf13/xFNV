#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderMenuBG : public ImageSpaceShader {
public:
	ImageSpaceShaderMenuBG();
	~ImageSpaceShaderMenuBG() override;

	void ReloadShaders() override;
};
