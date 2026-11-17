#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTHorizontalScramble : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTHorizontalScramble();
    ~ImageSpaceShaderFFTHorizontalScramble() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
