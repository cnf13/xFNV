#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderRestoreEDRAM : public ImageSpaceShader {
public:
	ImageSpaceShaderRestoreEDRAM();
	~ImageSpaceShaderRestoreEDRAM() override;

	void ReloadShaders() override {}
};
