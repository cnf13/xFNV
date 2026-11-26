#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderHDRBlendInShader : public ImageSpaceShader {
public:
    ImageSpaceShaderHDRBlendInShader();
    ~ImageSpaceShaderHDRBlendInShader() override;

    void ReloadShaders() override;
};
