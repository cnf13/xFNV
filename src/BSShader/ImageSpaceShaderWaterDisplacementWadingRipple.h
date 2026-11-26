#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterDisplacementWadingRipple : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterDisplacementWadingRipple();
    ~ImageSpaceShaderWaterDisplacementWadingRipple() override;

    void ReloadShaders() override;
    void SetState() override;
};
