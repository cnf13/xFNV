#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTDisplayWaterHeight : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTDisplayWaterHeight();
    ~ImageSpaceShaderFFTDisplayWaterHeight() override;

    void ReloadShaders() override;
};
