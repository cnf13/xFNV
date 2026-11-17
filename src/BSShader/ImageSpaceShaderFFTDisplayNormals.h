#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderFFTDisplayNormals : public ImageSpaceShader {
public:
    ImageSpaceShaderFFTDisplayNormals();
    ~ImageSpaceShaderFFTDisplayNormals() override;

    void ReloadShaders() override;
};
