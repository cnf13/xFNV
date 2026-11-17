#pragma once

#include "Fallout Shared/BGSPerkEntry.h"
#include "Reconstruction.hpp"

class BGSEntryPointFunctionData;
class TESCondition;

// The DATA chunk of an entry point perk entry (3 bytes): which entry point, which function (add, multiply, set, ...)
// and how many conditions the filter forms have.
struct EntryPointPerkEntryData {
	uint8_t cEntryPoint;
	uint8_t cFunction;
	uint8_t cNumConditions;
};

// A perk entry that changes a value at an entry point: Data, the function's data and the conditions of the filter
// forms (an array of Data.cNumConditions).
class BGSEntryPointPerkEntry : public BGSPerkEntry {
public:
	BGSEntryPointPerkEntry();

	bool CheckConditionFilters(uint32_t aiNumFilterForms, TESForm** apFilterForms) override;
	uint8_t GetFunction() override;
	void* GetFunctionData() override { return pFunctionData; }
	~BGSEntryPointPerkEntry() override;
	PERK_ENTRY_TYPE GetType() override;
	void ClearData() override;
	void InitItem(TESForm* apOwner) override;
	BGSPerkEntry* Duplicate() override;
	bool Compare(BGSPerkEntry* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;
	void ApplyPerkEntry(Actor* apActor, bool bAlt) override;
	void RemovePerkEntry(Actor* apActor, bool bAlt) override;

	// Data.cEntryPoint; the exe's shared byte getter at +8 (0x62F2F0), defined in PlayerCharacter.cpp.
	uint8_t GetEntryPoint();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	EntryPointPerkEntryData Data;
	BGSEntryPointFunctionData* pFunctionData;
	TESCondition* pConditionsArray;
};
inline constexpr fnvd::Vtable BGSEntryPointPerkEntry_vftable{};
