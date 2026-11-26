#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTNormals : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTNormals();
    ~ImageSpaceShaderWaterFFTNormals() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
