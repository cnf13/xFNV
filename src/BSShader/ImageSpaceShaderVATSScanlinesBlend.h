#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderVATSScanlinesBlend : public ImageSpaceShader {
public:
    ImageSpaceShaderVATSScanlinesBlend();
    ~ImageSpaceShaderVATSScanlinesBlend() override;

    void ReloadShaders() override;
};
