#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRLightAdapt : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRLightAdapt();
    ~ImageSpaceShaderHDRLightAdapt() override;

    void ReloadShaders() override;
};
