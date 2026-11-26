#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlendInShaderCinematic : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlendInShaderCinematic();
    ~ImageSpaceShaderHDRBlendInShaderCinematic() override;

    void ReloadShaders() override;
};
