#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderScanlinesBlend : public ImageSpaceShader {
public:
    ImageSpaceShaderScanlinesBlend();
    ~ImageSpaceShaderScanlinesBlend() override;

    void ReloadShaders() override;
};
