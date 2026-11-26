#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTDisplayWaterHeight : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTDisplayWaterHeight();
    ~ImageSpaceShaderWaterFFTDisplayWaterHeight() override;

    void ReloadShaders() override;
};
