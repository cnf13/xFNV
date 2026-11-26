#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTWaterSpectrum : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTWaterSpectrum();
    ~ImageSpaceShaderWaterFFTWaterSpectrum() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
