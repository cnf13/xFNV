#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTWaterSpectrum : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTWaterSpectrum();
	~ImageSpaceShaderFFTWaterSpectrum() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};
