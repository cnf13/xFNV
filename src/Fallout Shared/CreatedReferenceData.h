#pragma once

#include "Fallout Shared/ReferenceData.h"

// What kind of reference a save game created (TESSaveLoadGame::CreateReference switches on it).
enum TESCreatedReferenceType : int32_t {
	TES_CREATED_REFERENCE_NORMAL = 0,
	TES_CREATED_REFERENCE_ARROWPROJECTILE = 1,
	TES_CREATED_REFERENCE_MAGICPROJECTILE = 2,
	TES_CREATED_REFERENCE_PERSISTENT = 3,
};

// A created reference's type, base form ID and placement.
// IDA truncates RefData to a dword; the constructor initializes ReferenceData at +0x8,
// and TESSaveLoadGame::CreateChangedReference (0x8598D0) copies 0x24 bytes.
class CreatedReferenceData {
public:
	CreatedReferenceData();

	TESCreatedReferenceType eType;
	uint32_t iBoundID;
	ReferenceData RefData;
};
