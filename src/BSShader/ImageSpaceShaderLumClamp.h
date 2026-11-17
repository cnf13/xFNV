#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderLumClamp : public ImageSpaceShader {
public:
	ImageSpaceShaderLumClamp();
	~ImageSpaceShaderLumClamp() override;

	void ReloadShaders() override;
};
