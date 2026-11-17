#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Reconstruction.hpp"

// A form ID as three bytes: the file index and the form's number in it (the PDB's BGSSaveLoadStructs.cpp).
struct BGSNumericIDIndex {
	BGSNumericIDIndex();

	bool IsZero();
	uint32_t GetNumericID();
	void SetNumericID(uint32_t aiNumericID);

	uint8_t cData1;
	uint8_t cData2;
	uint8_t cData3;
};

// The form type and the size type of a saved form, in one byte (the PDB's BGSSaveLoadStructs.cpp).
class BGSSaveLoadFormInfo {
public:
	BGSSaveLoadFormInfo();

	ENUM_FORM_ID GetFormType();
	void SetFormType(ENUM_FORM_ID aeFormType);
	// The size type held in the top two bits (thiscall, ret 0, 0x864520; the loaders call it on
	// a header's FormInfo, the unloaded buffer on its first byte).
	SIZE_TYPE GetSizeType();
	// Sets the size type held in the top two bits.
	void SetSizeType(SIZE_TYPE aeSizeType);

	// Fills the form-type-to-save-index table from cSaveGameFormTypes.
	static void InitSaveGameFormTypes();

	uint8_t cData;
};

// The header of a saved form, packed to byte alignment (9 bytes): the form ID, the change flags, the form's type and
// size type and the save version. IDA names the fields kFormIDIndex, uiChangeFlags, ucFormInfo and ucVersion.
#pragma pack(push, 1)
struct BGSSaveLoadFormHeader {
	BGSSaveLoadFormHeader();

	BGSNumericIDIndex FormIDIndex;
	BGSChangeFlags iChangeFlags;
	BGSSaveLoadFormInfo FormInfo;
	uint8_t cVersion;
};
#pragma pack(pop)
