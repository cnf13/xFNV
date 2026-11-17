#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterSmoothHeightmap : public ImageSpaceShader {
public:
	WaterSmoothHeightmap();
	~WaterSmoothHeightmap() override;

	void ReloadShaders() override;
};
