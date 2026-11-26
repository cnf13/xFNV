#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample4LumClamp : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample4LumClamp();
    ~ImageSpaceShaderHDRDownSample4LumClamp() override;

    void ReloadShaders() override;
};
