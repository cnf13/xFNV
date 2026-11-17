#pragma once

#include "Fallout Shared/ReferenceData.h"

// A reference's original placement followed by its saved replacement placement.
// IDA truncates RefData to a dword; the constructor calls ReferenceData::Construct at +0x10,
// and TESSaveLoadGame::CreateChangedReference (0x8598D0) copies 0x2C bytes.
class MovedReferenceData {
public:
	MovedReferenceData();

	uint32_t iOriginalLocationID;
	NiPoint3 OriginalLoc;
	ReferenceData RefData;
};
