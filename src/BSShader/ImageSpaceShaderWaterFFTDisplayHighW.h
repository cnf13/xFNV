#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTDisplayHighW : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTDisplayHighW();
    ~ImageSpaceShaderWaterFFTDisplayHighW() override;

    void ReloadShaders() override;
};
