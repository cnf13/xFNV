#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlendCinematicAlphaMask : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlendCinematicAlphaMask();
    ~ImageSpaceShaderHDRBlendCinematicAlphaMask() override;

    void ReloadShaders() override;
};
