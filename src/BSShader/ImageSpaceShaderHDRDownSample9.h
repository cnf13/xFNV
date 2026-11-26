#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRDownSample9 : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRDownSample9();
    ~ImageSpaceShaderHDRDownSample9() override;

    void ReloadShaders() override;
};
