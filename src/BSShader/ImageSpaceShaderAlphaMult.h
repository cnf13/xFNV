#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderAlphaMult : public ImageSpaceShader {
public:
	ImageSpaceShaderAlphaMult();
	~ImageSpaceShaderAlphaMult() override;

	void ReloadShaders() override;
};
