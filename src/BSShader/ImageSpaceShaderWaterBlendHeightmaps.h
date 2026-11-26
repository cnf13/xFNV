#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterBlendHeightmaps : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterBlendHeightmaps();
    ~ImageSpaceShaderWaterBlendHeightmaps() override;

    void ReloadShaders() override;
};
