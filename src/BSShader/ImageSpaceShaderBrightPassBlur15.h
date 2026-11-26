#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBrightPassBlur15 : public ImageSpaceShader {
public:
    ImageSpaceShaderBrightPassBlur15();
    ~ImageSpaceShaderBrightPassBlur15() override;

    void ReloadShaders() override;
};
