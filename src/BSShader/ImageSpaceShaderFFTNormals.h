#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTNormals : public ImageSpaceShader {
public:
	ImageSpaceShaderFFTNormals();
	~ImageSpaceShaderFFTNormals() override;

	void ReloadShaders() override;
	bool LoadStagesAndPasses() override;
};
