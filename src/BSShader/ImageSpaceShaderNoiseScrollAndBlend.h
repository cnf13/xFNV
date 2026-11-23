#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderNoiseScrollAndBlend : public ImageSpaceShader {
public:
    ImageSpaceShaderNoiseScrollAndBlend();
    ~ImageSpaceShaderNoiseScrollAndBlend() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
