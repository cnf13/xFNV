#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterDisplacementRainRipple : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterDisplacementRainRipple();
    ~ImageSpaceShaderWaterDisplacementRainRipple() override;

    void ReloadShaders() override;
};
