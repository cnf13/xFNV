#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderInterfaceScanlinesBlend : public ImageSpaceShader {
public:
    ImageSpaceShaderInterfaceScanlinesBlend();
    ~ImageSpaceShaderInterfaceScanlinesBlend() override;

    void ReloadShaders() override;
};
