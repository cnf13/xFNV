#pragma once

#include "Fallout Shared/TESObjectARMO.h"
#include "Reconstruction.hpp"

// An armor addon (ARMA): an armor form with the same members (both allocate 0x190 bytes).
class TESObjectARMA : public TESObjectARMO {
public:
	TESObjectARMA();
	~TESObjectARMA() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitItem() override;
	bool IsArmorAddon() override { return true; }
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESObjectARMO::LoadGame;
	using TESObjectARMO::Save;
	using TESObjectARMO::SaveGame;
};
inline constexpr fnvd::Vtable TESObjectARMA_vftable{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_BGSBipedModelList{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_BGSEquipType{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_BGSRepairItemList{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESBipedModelForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESEnchantableForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESHealthForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESObjectARMA_vftable_TESWeightForm{};
