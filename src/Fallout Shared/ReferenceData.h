#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The saved location and orientation of a reference.
class ReferenceData {
public:
	// The constructor deliberately leaves Loc and Angle unset, as the game does.
	ReferenceData();

	uint32_t iLocationID;
	NiPoint3 Loc;
	NiPoint3 Angle;
};
