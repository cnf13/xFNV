#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderWaterFFTDisplayNormals : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterFFTDisplayNormals();
    ~ImageSpaceShaderWaterFFTDisplayNormals() override;

    void ReloadShaders() override;
};
