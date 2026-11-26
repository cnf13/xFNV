#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTHorizontalButterfly : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTHorizontalButterfly();
    ~ImageSpaceShaderWaterFFTHorizontalButterfly() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
