#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTVerticalButterfly : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTVerticalButterfly();
    ~ImageSpaceShaderWaterFFTVerticalButterfly() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
