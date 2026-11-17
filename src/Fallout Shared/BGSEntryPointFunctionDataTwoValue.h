#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Reconstruction.hpp"

// The two values of BGSEntryPointFunctionDataTwoValue as Save and Load swap and copy them (the PDB's local
// TwoValues saveData of both, BGSEntryPointFunction.cpp).
struct TwoValues {
	void Endian();

	float fValue1;
	float fValue2;
};

// Entry point function data holding two floats (IDA's fValue[2]).
class BGSEntryPointFunctionDataTwoValue : public BGSEntryPointFunctionData {
public:
	BGSEntryPointFunctionDataTwoValue();
	~BGSEntryPointFunctionDataTwoValue() override;

	ENTRY_POINT_FUNCTION_DATA GetType() override;
	BGSEntryPointFunctionData* Duplicate() override;
	bool Compare(BGSEntryPointFunctionData* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;

	float fValue1;
	float fValue2;
};
inline constexpr fnvd::Vtable BGSEntryPointFunctionDataTwoValue_vftable{};
