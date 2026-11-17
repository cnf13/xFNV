#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderMap : public ImageSpaceShader {
public:
	ImageSpaceShaderMap();
	~ImageSpaceShaderMap() override;

	void ReloadShaders() override;
};
