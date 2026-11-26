#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur11 : public ImageSpaceShader {
public:
    ImageSpaceShaderBrightPassBlur11();
    ~ImageSpaceShaderBrightPassBlur11() override;

    void ReloadShaders() override;
};
