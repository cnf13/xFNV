#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterSmoothHeightmap : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterSmoothHeightmap();
    ~ImageSpaceShaderWaterSmoothHeightmap() override;

    void ReloadShaders() override;
};
