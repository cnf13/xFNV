#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderBlur7 : public ImageSpaceShader {
public:
    ImageSpaceShaderBlur7();
    ~ImageSpaceShaderBlur7() override;

    void ReloadShaders() override;
};
