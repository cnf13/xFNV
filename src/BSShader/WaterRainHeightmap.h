#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterRainHeightmap : public ImageSpaceShader {
public:
	WaterRainHeightmap();
	~WaterRainHeightmap() override;

	void ReloadShaders() override;
};
