#pragma once

#include "BSShader/ImageSpaceShader.h"

// The noise normal-map post-processing shader. InitEffects allocates 0xD0 bytes.
class ImageSpaceShaderNoiseNormalmap : public ImageSpaceShader {
public:
    ImageSpaceShaderNoiseNormalmap();
    ~ImageSpaceShaderNoiseNormalmap() override;

    void ReloadShaders() override;
    bool LoadStagesAndPasses() override;
};
