#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterWadingHeightmap : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterWadingHeightmap();
    ~ImageSpaceShaderWaterWadingHeightmap() override;

    void ReloadShaders() override;
    void SetState() override;
};
