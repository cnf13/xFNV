#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterDisplacementWadingRipple : public ImageSpaceShader {
public:
	WaterDisplacementWadingRipple();
	~WaterDisplacementWadingRipple() override;

	void ReloadShaders() override;
	void SetState() override;
};
