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
class CreatedReferenceData {
public:
	CreatedReferenceData();

	TESCreatedReferenceType eType;
	uint32_t iBoundID;
	ReferenceData RefData;
};
