#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderRadialBlurHigh : public ImageSpaceShader {
public:
    ImageSpaceShaderRadialBlurHigh();
    ~ImageSpaceShaderRadialBlurHigh() override;

    void ReloadShaders() override;
};
