#pragma once

#include "BSShader/TallGrassShaderProperty.h"

// The grass shader parameters and density samples for a terrain area.
class TESGrassAreaParam {
public:
    TESGrassAreaParam();

    TallGrassShaderProperty::GrassParam kGrassParam;
    float pfDensity[9];
};
