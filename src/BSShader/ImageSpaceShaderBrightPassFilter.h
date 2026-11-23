#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassFilter : public ImageSpaceShader {
public:
    ImageSpaceShaderBrightPassFilter();
    ~ImageSpaceShaderBrightPassFilter() override;

    void ReloadShaders() override;
};
