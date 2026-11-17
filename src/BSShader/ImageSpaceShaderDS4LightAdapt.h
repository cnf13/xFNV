#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDS4LightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderDS4LightAdapt();
	~ImageSpaceShaderDS4LightAdapt() override;

	void ReloadShaders() override;
};
