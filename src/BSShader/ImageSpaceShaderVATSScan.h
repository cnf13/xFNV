#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderVATSScan : public ImageSpaceShader {
public:
    ImageSpaceShaderVATSScan();
    ~ImageSpaceShaderVATSScan() override;

    void ReloadShaders() override;
};
