#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur13 : public ImageSpaceShader {
public:
    ImageSpaceShaderBrightPassBlur13();
    ~ImageSpaceShaderBrightPassBlur13() override;

    void ReloadShaders() override;
};
