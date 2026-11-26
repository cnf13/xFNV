#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample4 : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample4();
    ~ImageSpaceShaderHDRDownSample4() override;

    void ReloadShaders() override;
};
