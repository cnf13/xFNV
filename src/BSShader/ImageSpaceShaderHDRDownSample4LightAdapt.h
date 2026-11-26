#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample4LightAdapt : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample4LightAdapt();
    ~ImageSpaceShaderHDRDownSample4LightAdapt() override;

    void ReloadShaders() override;
};
