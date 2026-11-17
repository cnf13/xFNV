#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlendCinematic : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlendCinematic();
    ~ImageSpaceShaderHDRBlendCinematic() override;

    void ReloadShaders() override;
};
