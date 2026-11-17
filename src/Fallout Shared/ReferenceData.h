#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// The saved location and orientation of a reference.
class ReferenceData {
public:
	// Loc and Angle stay unset, as in the exe.
	ReferenceData();

	uint32_t iLocationID;
	NiPoint3 Loc;
	NiPoint3 Angle;
};
