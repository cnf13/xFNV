#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur9 : public ImageSpaceShader {
public:
    ImageSpaceShaderBlur9();
    ~ImageSpaceShaderBlur9() override;

    void ReloadShaders() override;
};
