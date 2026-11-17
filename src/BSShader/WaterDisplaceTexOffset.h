#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterDisplaceTexOffset : public ImageSpaceShader {
public:
    WaterDisplaceTexOffset();
    ~WaterDisplaceTexOffset() override;

    void ReloadShaders() override;
};
