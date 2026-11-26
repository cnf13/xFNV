#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterDisplacementNormals : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterDisplacementNormals();
    ~ImageSpaceShaderWaterDisplacementNormals() override;

    void ReloadShaders() override;
};
