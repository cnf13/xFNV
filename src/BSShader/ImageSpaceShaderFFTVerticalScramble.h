#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTVerticalScramble : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTVerticalScramble();
    ~ImageSpaceShaderFFTVerticalScramble() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
