#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

// A clothing base form. Its members are not declared yet.
class TESObjectCLOT : public TESBoundObject,
					  public TESFullName,
					  public TESScriptableForm,
					  public TESEnchantableForm,
					  public TESValueForm,
					  public TESWeightForm,
					  public TESBipedModelForm,
					  public BGSDestructibleObjectForm,
					  public BGSEquipType {
public:
	TESObjectCLOT();
	~TESObjectCLOT() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;
};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_BGSEquipType{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESBipedModelForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESEnchantableForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectCLOT_vftable{};
