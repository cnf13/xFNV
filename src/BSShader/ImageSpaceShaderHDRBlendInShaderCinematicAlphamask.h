#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlendInShaderCinematicAlphamask : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlendInShaderCinematicAlphamask();
    ~ImageSpaceShaderHDRBlendInShaderCinematicAlphamask() override;

    void ReloadShaders() override;
};
