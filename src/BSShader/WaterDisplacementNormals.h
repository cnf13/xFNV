#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterDisplacementNormals : public ImageSpaceShader {
public:
	WaterDisplacementNormals();
	~WaterDisplacementNormals() override;

	void ReloadShaders() override;
};
