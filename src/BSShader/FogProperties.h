#pragma once

#include "BSShader/FogParameters.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

// The fog a shader is given: its distances and power, and its color. IDA's names; the PDB has no such class.
class FogProperties {
public:
	// IDA names; not in the PDB.
	::FogParameters FogParameters;
	NiColorA Color;
};
