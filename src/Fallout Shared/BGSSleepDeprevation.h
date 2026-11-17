#pragma once

#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class SpellItem;

// The sleep deprivation a stage starts at and the sickness it gives.
struct BGSSleepDeprevationStageData {
	// Swaps both dwords, the sickness then holding a form ID.
	void Endian();
	// Zeroes both dwords.
	void Clear();

	uint32_t iSleep;
	SpellItem* pSickness;
};

// One stage of hardcore sleep deprivation (SLPD).
class BGSSleepDeprevationStage : public TESForm {
public:
	BGSSleepDeprevationStage();
	~BGSSleepDeprevationStage() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// The stage's record data, at +0x18: the exe's shared one-field getter (thiscall, ret 0,
	// 0x500940, folded with BGSListForm::GetFormList).
	BGSSleepDeprevationStageData* GetData();
	// Folded one-field getter (IDA: NiShadeProperty::GetShaderPropertyType), 0x441110.
	SpellItem* GetSickness();

protected:
	// Removes apStage from the sorted stages array.
	static void RemoveStage(BGSSleepDeprevationStage* apStage);
	// Adds apStage to the sorted stages array (cdecl, 0x402550; IDA types the argument BSBound* and returns the new
	// count).
	static void AddStage(BGSSleepDeprevationStage* apNewStage);

public:
	static BGSSleepDeprevationStage** AllocateStages(uint32_t aiStagesCount);
	// The aiIndex-th stage of the sorted array, or null past the end.
	static BGSSleepDeprevationStage* GetStage(uint32_t aiIndex);
	// The highest stage whose sleep deprivation afRads reaches, or null.
	static BGSSleepDeprevationStage* FindStageForRadLevel(float afRads);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BGSSleepDeprevationStageData data;

	static inline int32_t iNumStages;
	// The stages sorted by sleep deprivation, iNumStages of them, allocated by AddStage and RemoveStage (the PDB's
	// pStagesArray).
	static inline BGSSleepDeprevationStage** pStagesArray;
};
inline constexpr fnvd::Vtable BGSSleepDeprevationStage_vftable{};
