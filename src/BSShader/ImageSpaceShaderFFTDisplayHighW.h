#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTDisplayHighW : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTDisplayHighW();
    ~ImageSpaceShaderFFTDisplayHighW() override;

    void ReloadShaders() override;
};
