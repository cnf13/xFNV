#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTDisplayAmplitude : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTDisplayAmplitude();
    ~ImageSpaceShaderFFTDisplayAmplitude() override;

    void ReloadShaders() override;
};
