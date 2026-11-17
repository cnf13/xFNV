#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTHorizontalButterfly : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTHorizontalButterfly();
	~ImageSpaceShaderFFTHorizontalButterfly() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};
