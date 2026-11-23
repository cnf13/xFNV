#pragma once

#include "BSShader/ImageSpaceShader.h"

// The image-space shader that draws the sun's beams through the stencil.
class ImageSpaceShaderSunbeams : public ImageSpaceShader {
public:
    ImageSpaceShaderSunbeams();
    ~ImageSpaceShaderSunbeams() override;

    // Loads the sunbeams shaders.
    void ReloadShaders() override;
    void SetState() override;
};
