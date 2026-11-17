#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderCinematic : public ImageSpaceShader {
public:
	ImageSpaceShaderCinematic();
	~ImageSpaceShaderCinematic() override;

	void ReloadShaders() override;
};
