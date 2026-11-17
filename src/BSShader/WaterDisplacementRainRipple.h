#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterDisplacementRainRipple : public ImageSpaceShader {
public:
    WaterDisplacementRainRipple();
    ~WaterDisplacementRainRipple() override;

    void ReloadShaders() override;
};
