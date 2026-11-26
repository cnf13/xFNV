#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample16 : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample16();
    ~ImageSpaceShaderHDRDownSample16() override;

    void ReloadShaders() override;
};
