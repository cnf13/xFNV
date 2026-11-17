#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDepthOfFieldMask : public ImageSpaceShader {
public:
	ImageSpaceShaderDepthOfFieldMask();
	~ImageSpaceShaderDepthOfFieldMask() override;

	void ReloadShaders() override;
};
