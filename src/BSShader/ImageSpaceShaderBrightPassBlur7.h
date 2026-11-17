#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur7 : public ImageSpaceShader {
public:
	ImageSpaceShaderBrightPassBlur7();
	~ImageSpaceShaderBrightPassBlur7() override;

	void ReloadShaders() override;
};
