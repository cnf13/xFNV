#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterBlendHeightmaps : public ImageSpaceShader {
public:
	WaterBlendHeightmaps();
	~WaterBlendHeightmaps() override;

	void ReloadShaders() override;
};
