#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderRadialBlurMedium : public ImageSpaceShader {
public:
    ImageSpaceShaderRadialBlurMedium();
    ~ImageSpaceShaderRadialBlurMedium() override;

    void ReloadShaders() override;
};
