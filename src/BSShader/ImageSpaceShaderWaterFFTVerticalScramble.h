#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTVerticalScramble : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTVerticalScramble();
    ~ImageSpaceShaderWaterFFTVerticalScramble() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
