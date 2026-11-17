#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderStencilCopyNoTextureNoDepth : public ImageSpaceShader {
public:
	ImageSpaceShaderStencilCopyNoTextureNoDepth();
	~ImageSpaceShaderStencilCopyNoTextureNoDepth() override;

	void ReloadShaders() override;
	void SetState() override;

	// The stencil reference the shader tests against (TESWaterSystem::UpdatePlaceableWater sets it per water group).
	static inline uint32_t uiStencilRef;
};
