#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDS16LumClamp : public ImageSpaceShader {
public:
	ImageSpaceShaderDS16LumClamp();
	~ImageSpaceShaderDS16LumClamp() override;

	void ReloadShaders() override;
};
