#pragma once

#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class BGSDehydrationStage;
class SpellItem;

// The dehydration a stage starts at and the sickness it gives.
struct BGSDehydrationStageData {
	// The exe's shared byte swap of a form's data (EXTERIOR_DATA::Endian, thiscall,
	// 0x462230; the exe folds it with other forms' data).
	void Endian();
	// Zeroes both fields.
	void Clear();

	uint32_t iH20;
	SpellItem* pSickness;
};

// One stage of hardcore dehydration (DEHY).
class BGSDehydrationStage : public TESForm {
public:
	BGSDehydrationStage();
	~BGSDehydrationStage() override;

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
	// The exe's shared "this + 0x18" getter (BGSListForm::GetFormList's address 0x500940,
	// thiscall, ret 0), which the exe calls out of line.
	BGSDehydrationStageData* GetData();

protected:
	// Removes apStage from the sorted stages array.
	static void RemoveStage(BGSDehydrationStage* apStage);

public:
	// The highest stage whose dehydration afRads reaches, or null.
	static BGSDehydrationStage* FindStageForRadLevel(float afRads);
	// Inserts apStage into the sorted stages array.
	static int32_t AddStage(BGSDehydrationStage* apNewStage);
	// IDA name GetStage; not in the PDB. The stage at auiIndex, or null (cdecl, 0x401930).
	static BGSDehydrationStage* GetStage(uint32_t auiIndex);
	static BGSDehydrationStage** AllocateStages(uint32_t aiStagesCount);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BGSDehydrationStageData data;

	static inline int32_t iNumStages;
	static inline BGSDehydrationStage** pStagesArray;
};
inline constexpr fnvd::Vtable BGSDehydrationStage_vftable{};
