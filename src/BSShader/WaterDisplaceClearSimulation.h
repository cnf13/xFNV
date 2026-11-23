#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterDisplaceClearSimulation : public ImageSpaceShader {
public:
    WaterDisplaceClearSimulation();
    ~WaterDisplaceClearSimulation() override;

    void ReloadShaders() override;
};
