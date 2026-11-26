#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTDisplayAmplitude : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTDisplayAmplitude();
    ~ImageSpaceShaderWaterFFTDisplayAmplitude() override;

    void ReloadShaders() override;
};
