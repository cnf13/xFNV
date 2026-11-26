#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class ImageSpaceShaderWaterDisplacementClearSimulation : public ImageSpaceShader {
public:
    ImageSpaceShaderWaterDisplacementClearSimulation();
    ~ImageSpaceShaderWaterDisplacementClearSimulation() override;

    void ReloadShaders() override;
};
