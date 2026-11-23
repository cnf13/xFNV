#pragma once

#include "BSShader/FogParameters.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

// The fog a shader is given: its distances and power, and its color.
class FogProperties {
public:
    ::FogParameters FogParameters;
    NiColorA Color;
};
