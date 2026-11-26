#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterDisplacementTexOffset : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterDisplacementTexOffset();
    ~ImageSpaceShaderWaterDisplacementTexOffset() override;

    void ReloadShaders() override;
};
