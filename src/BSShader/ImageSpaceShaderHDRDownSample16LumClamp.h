#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample16LumClamp : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample16LumClamp();
    ~ImageSpaceShaderHDRDownSample16LumClamp() override;

    void ReloadShaders() override;
};
