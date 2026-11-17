#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Reconstruction.hpp"

// A head part form (HDPT).
class BGSHeadPart : public TESForm, public TESFullName, public TESModelTextureSwap {
public:
	BGSHeadPart();
	~BGSHeadPart() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	BSSimpleList<BGSHeadPart*>* GetExtraPartList();
	// Adds a part to the extra part list.
	void AddExtraPart(BGSHeadPart* apPart);
	bool IsPlayable();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint8_t cFlags;
	uint8_t padding45[3];
	// the parts that come with this one (IDA types the items void*; the PDB name is ExtraParts)
	BSSimpleList<BGSHeadPart*> ExtraParts;
};
inline constexpr fnvd::Vtable BGSHeadPart_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable BGSHeadPart_vftable_TESFullName{};
inline constexpr fnvd::Vtable BGSHeadPart_vftable{};
