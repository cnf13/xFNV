#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur9 : public ImageSpaceShader {
public:
    ImageSpaceShaderBrightPassBlur9();
    ~ImageSpaceShaderBrightPassBlur9() override;

    void ReloadShaders() override;
};
