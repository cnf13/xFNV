#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderDoubleVision : public ImageSpaceShader {
public:
    ImageSpaceShaderDoubleVision();
    ~ImageSpaceShaderDoubleVision() override;

    void ReloadShaders() override;
};
