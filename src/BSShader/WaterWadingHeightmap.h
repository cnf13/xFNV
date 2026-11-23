#pragma once

#include "BSShader/ImageSpaceShader.h"

// A water image-space shader.
class WaterWadingHeightmap : public ImageSpaceShader {
public:
    WaterWadingHeightmap();
    ~WaterWadingHeightmap() override;

    void ReloadShaders() override;
    void SetState() override;
};
