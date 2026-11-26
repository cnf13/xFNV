#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample16LightAdapt : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample16LightAdapt();
    ~ImageSpaceShaderHDRDownSample16LightAdapt() override;

    void ReloadShaders() override;
};
