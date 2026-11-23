#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderRefraction : public ImageSpaceShader {
public:
    ImageSpaceShaderRefraction();
    ~ImageSpaceShaderRefraction() override;

    void ReloadShaders() override;
};
