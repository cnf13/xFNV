#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLoadScreenType.h"
#include "Fallout Shared/TESTexture.h"

// One location a load screen shows for: its form ID, its world and its cell.
struct LOAD_FORM_DATA {
	void Endian();

	uint32_t iFormID;
	uint32_t iWorldID;
	uint32_t iCellKey;
};

// A load screen form (LSCR).
class TESLoadScreen : public TESForm, public TESTexture, public TESDescription {
public:
	TESLoadScreen();
	~TESLoadScreen() override;

	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	const char* GetDescription(TESForm* apForm, CHUNK_ID aeChunk) override;

	using TESForm::Save;

	void ClearLoadFormList();
	// The load screen type (pLoadScreenType).
	TESLoadScreenType* GetLoadScreenType();
	LoadScreenType_Data* GetLoadScreenTypeData();
	// The type's data type (data at +0x18), LSDT_NONE without a type.
	LOAD_SCREEN_DATA_TYPE GetLoadScreenTypeDataType();
	// Returns whether the main-menu form flag is set.
	bool GetDisplaysOnMainMenu();
	bool IsNonLocationSpecific();
	// apForm is a cell, reference or worldspace: the function switches on its form type.
	char IsValidForCell(TESForm* apForm);

	// True when only location specific screens may show (bLocationSpecificScreensOnly). Defined in LoadingMenu.cpp.
	static bool QUseLocationSpecificScreensOnly();

	// The forms (locations) the screen shows for.
	BSSimpleList<LOAD_FORM_DATA*> LoadFormList;
	TESLoadScreenType* pLoadScreenType;
	BSStringT<char> cDescText;
};
