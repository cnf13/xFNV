#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"

class TESLevItem;

// Entry point function data holding a leveled item list.
class BGSEntryPointFunctionDataLeveledList : public BGSEntryPointFunctionData {
public:
	BGSEntryPointFunctionDataLeveledList();
	~BGSEntryPointFunctionDataLeveledList() override;

	ENTRY_POINT_FUNCTION_DATA GetType() override;
	BGSEntryPointFunctionData* Duplicate() override;
	bool Compare(BGSEntryPointFunctionData* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;
	void InitItem(TESForm* apOwner) override;

	TESLevItem* pLeveledList;
};
