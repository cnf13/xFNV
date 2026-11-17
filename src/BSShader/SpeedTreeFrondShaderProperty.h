#pragma once

#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// The property of a SpeedTree model's fronds. Only the RTTI the 1.x pass selection tests is declared so far.
class SpeedTreeFrondShaderProperty : public SpeedTreeShaderLightingProperty {
public:
	static NiRTTI ms_RTTI;
};
