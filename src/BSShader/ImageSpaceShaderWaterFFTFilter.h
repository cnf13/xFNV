#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTFilter : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTFilter();
    ~ImageSpaceShaderWaterFFTFilter() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
