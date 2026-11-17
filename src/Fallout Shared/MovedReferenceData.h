#pragma once

#include "Fallout Shared/ReferenceData.h"

// A reference's original placement followed by its saved replacement placement.
class MovedReferenceData {
public:
	MovedReferenceData();

	uint32_t iOriginalLocationID;
	NiPoint3 OriginalLoc;
	ReferenceData RefData;
};
