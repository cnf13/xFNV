#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDS4LumClamp : public ImageSpaceShader {
public:
    ImageSpaceShaderDS4LumClamp();
    ~ImageSpaceShaderDS4LumClamp() override;

    void ReloadShaders() override;
};
