#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Reconstruction.hpp"

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

	// fValue (same code as DecalData::GetDecalMaxWidth, thiscall, ST0, 0x6B9130).
	float GetValue();

	float fValue;
};
inline constexpr fnvd::Vtable BGSEntryPointFunctionDataOneValue_vftable{};
