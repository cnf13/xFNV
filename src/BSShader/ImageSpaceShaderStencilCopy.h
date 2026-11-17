#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderStencilCopy : public ImageSpaceShader {
public:
	ImageSpaceShaderStencilCopy();
	~ImageSpaceShaderStencilCopy() override;

	void ReloadShaders() override;
	void SetState() override;

	// The stencil reference value the copy compares against.
	static inline uint32_t uiStencilRef = 128;
};
