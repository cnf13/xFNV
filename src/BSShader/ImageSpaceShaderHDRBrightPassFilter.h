#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBrightPassFilter : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBrightPassFilter();
    ~ImageSpaceShaderHDRBrightPassFilter() override;

    void ReloadShaders() override;
};
