#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderCopyRenderTarget1 : public ImageSpaceShader {
public:
	ImageSpaceShaderCopyRenderTarget1();
	~ImageSpaceShaderCopyRenderTarget1() override;

	void ReloadShaders() override;
	void SetState() override;
};
