#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTVerticalButterfly : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTVerticalButterfly();
	~ImageSpaceShaderFFTVerticalButterfly() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};
