#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderNULL : public ImageSpaceShader {
public:
    ImageSpaceShaderNULL();
    ~ImageSpaceShaderNULL() override;

    void ReloadShaders() override;
};
