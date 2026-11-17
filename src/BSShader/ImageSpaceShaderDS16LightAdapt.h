#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDS16LightAdapt : public ImageSpaceShader {
public:
	ImageSpaceShaderDS16LightAdapt();
	~ImageSpaceShaderDS16LightAdapt() override;

	void ReloadShaders() override;
};
