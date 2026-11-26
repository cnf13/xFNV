#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRLumClamp : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRLumClamp();
    ~ImageSpaceShaderHDRLumClamp() override;

    void ReloadShaders() override;
};
