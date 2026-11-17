#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"

// Entry point function data holding one float.
class BGSEntryPointFunctionDataOneValue : public BGSEntryPointFunctionData {
public:
	BGSEntryPointFunctionDataOneValue();
	~BGSEntryPointFunctionDataOneValue() override;

	ENTRY_POINT_FUNCTION_DATA GetType() override;
	BGSEntryPointFunctionData* Duplicate() override;
	bool Compare(BGSEntryPointFunctionData* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;

	float GetValue();

	float fValue;
};
