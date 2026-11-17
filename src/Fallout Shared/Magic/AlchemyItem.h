#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

class SpellItem;
class TESSound;

// A potion's ENIT data: the cost override, the flags, the addiction and its chance, and the consumption sound.
struct AlchemyItemData {
	// Byte-swaps the cost, the addiction, its chance and the sound read from a big-endian file.
	void Endian();

	int32_t iCostOverride;
	// The exe's Endian leaves this field alone (a byte of flags).
	uint8_t iFlags;
	uint8_t padding05[3];
	SpellItem* pAddictionItem;
	float fAddictionChance;
	TESSound* pConsumptionSound;
};

// The base form of potions, food and drink.
class AlchemyItem : public MagicItemObject,
					public TESModelTextureSwap,
					public TESIcon,
					public BGSMessageIcon,
					public TESScriptableForm,
					public TESWeightForm,
					public BGSEquipType,
					public BGSDestructibleObjectForm,
					public BGSPickupPutdownSounds {
public:
	// The bits of the data's flags (PDB names and values; the exe tests them as a dword, 0x4029E1).
	enum AlchemyFlag : int32_t {
		COST_OVERRIDE = 0x1,
		FOOD_ITEM = 0x2,
		MEDICINE = 0x4,
	};

	AlchemyItem();
	~AlchemyItem() override;

	void InitializeData() override;
	void ClearData() override;
	void InitItem() override;
	bool IsAutoCalc() override;
	void SetAutoCalc(bool abValue) override;
	MagicSystem::SpellType GetSpellType() override { return MagicSystem::POTION; }
	bool IsMedicine() override { return AlchemyItem::IsFlagSet(MEDICINE); }
	bool IsFood() override;
	float GetCost(Actor* apActor) override;

protected:
	void Endian() override;
	CHUNK_ID GetChunkID() override;
	void* GetData() override { return &data; }
	uint32_t GetDataSize() override;
	bool CompareMagicItemData(MagicItem* apCompare) override;
	void CopyMagicItemData(MagicItem* apCopy) override;
	void SaveMagicItemComponents() override;
	void SaveMagicItemData() override;
	void LoadMagicItemChunk(TESFile* apFile, CHUNK_ID apChunkID) override;

public:
	// Whether any of aeFlag is set in the data flags.
	bool IsFlagSet(AlchemyFlag aiFlag);
	void SetFlag(AlchemyFlag aiFlag, bool abValue);
	// The data's cost override (thiscall, ret 0, 0x4036B0: the exe's one-field getter of +0xB8, shared with
	// TES::QPlaceableWaterCount, TESObjectLIGH::GetSound and DialoguePackage::GetTalkingActivator).
	int32_t GetCostOverride() { return data.iCostOverride; }
	// The addiction spell (thiscall, ret, 0x4030B0; folded with NiGeometry::GetShader).
	SpellItem* GetAddictionItem();
	// Sets the addiction spell (thiscall, ret 4, 0x874460; folded with NiGeometry::SetShader).
	void SetAddictionItem(SpellItem* apObject);
	// The consumption sound (thiscall, ret, 0x9ACCE0; folded with Explosion::GetOwner).
	TESSound* GetConsumptionSound();
	// Sets the consumption sound (thiscall, ret 4, 0x4030D0; folded with
	// BSMasterParticleSystem::SetEditorIndex).
	void SetConsumptionSound(TESSound* apObject);
	bool IsWater();

	// The bases' operator deletes are ambiguous: the same free (ourFree) as theirs.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	AlchemyItemData data;
	// The icon the message shown when it is used carries.
	TESIcon MessageIcon;
};
inline constexpr fnvd::Vtable AlchemyItem_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_BGSEquipType{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_TESIcon{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_EffectItemList{};
inline constexpr fnvd::Vtable AlchemyItem_vftable_MagicItem{};
inline constexpr fnvd::Vtable AlchemyItem_vftable{};
