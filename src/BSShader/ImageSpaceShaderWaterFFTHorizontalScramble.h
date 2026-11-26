#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTHorizontalScramble : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTHorizontalScramble();
    ~ImageSpaceShaderWaterFFTHorizontalScramble() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
