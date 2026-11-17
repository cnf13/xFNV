#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderLightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderLightAdapt();
	~ImageSpaceShaderLightAdapt() override;

	void ReloadShaders() override;
};
