#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterRainHeightmap : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterRainHeightmap();
    ~ImageSpaceShaderWaterRainHeightmap() override;

    void ReloadShaders() override;
};
