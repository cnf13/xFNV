#pragma once

#include "Fallout Shared/BGSEntryPointFunctionData.h"
#include "Fallout Shared/Script.h"
#include "Reconstruction.hpp"

// Entry point function data for an activate choice: the label, the script it runs and its flags (IDA's pLabel,
// kScript and usFlags).
class BGSEntryPointFunctionDataActivateChoice : public BGSEntryPointFunctionData {
public:
	BGSEntryPointFunctionDataActivateChoice();
	~BGSEntryPointFunctionDataActivateChoice() override;

	ENTRY_POINT_FUNCTION_DATA GetType() override;
	BGSEntryPointFunctionData* Duplicate() override;
	bool Compare(BGSEntryPointFunctionData* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;
	void InitItem(TESForm* apOwner) override;

	void InitializeData();
	// Frees the label and stores a copy of apLabel, or null.
	void SetLabel(const char* apLabel);
	// &ActivateChoiceScript (same code as NiObjectNET::GetName, thiscall, 0x413F40).
	Script* GetScript();
	// Whether the script runs at once instead of offering the choice (same code as
	// TESChallenge::GetStartsLocked, thiscall, 0x5F5930).
	uint32_t GetRunImmediately();

	char* pActivateChoiceLabel;
	Script ActivateChoiceScript;
	uint16_t cFlags;
};
inline constexpr fnvd::Vtable BGSEntryPointFunctionDataActivateChoice_vftable{};
