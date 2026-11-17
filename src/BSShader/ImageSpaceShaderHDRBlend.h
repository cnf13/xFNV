#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlend : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlend();
    ~ImageSpaceShaderHDRBlend() override;

    void ReloadShaders() override;
};
