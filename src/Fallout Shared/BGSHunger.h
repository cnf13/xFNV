#pragma once

#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class SpellItem;

// The hunger a stage starts at and the sickness it gives.
struct BGSHungerStageData {
	void Endian();
	// Zeroes both dwords.
	void Clear();

	uint32_t iFOOD;
	SpellItem* pSickness;
};

// One stage of hardcore hunger (HUNG).
class BGSHungerStage : public TESForm {
public:
	BGSHungerStage();
	~BGSHungerStage() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Folded one-field getter (IDA: NiShadeProperty::GetShaderPropertyType), 0x441110.
	SpellItem* GetSickness();
	// The stage's data.
	BGSHungerStageData* GetData() { return &data; }

protected:
	// Inserts apStage into the sorted stages array.
	static void AddStage(BGSHungerStage* apNewStage);
	// Removes apStage from the sorted stages array.
	static void RemoveStage(BGSHungerStage* apStage);

public:
	// The highest stage whose hunger afRads reaches, or null.
	static BGSHungerStage* FindStageForRadLevel(float afRads);
	// The stage at aiIndex in the sorted stage array, or null past its end.
	static BGSHungerStage* GetStage(uint32_t aiIndex);
	// A zero-filled array of aiCount stage pointers, or null for none.
	static BGSHungerStage** AllocateStages(uint32_t aiStagesCount);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BGSHungerStageData data;

	static inline int32_t iNumStages;
	// The stages sorted by their hunger level.
	static inline BGSHungerStage** pStagesArray;
};
inline constexpr fnvd::Vtable BGSHungerStage_vftable{};
