#pragma once

#include "BSShader/ImageSpaceShader.h"

#include <stdint.h>

// The image-space copy shader; it adds no instance data to ImageSpaceShader.
class ImageSpaceShaderCopy : public ImageSpaceShader {
public:
    enum : int32_t {
        VPARAM_COUNT = 0x0,
    };

    ImageSpaceShaderCopy();
    ~ImageSpaceShaderCopy() override;

    void ReloadShaders() override;
};
