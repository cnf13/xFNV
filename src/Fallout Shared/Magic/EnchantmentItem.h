#pragma once

#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Reconstruction.hpp"

class ExtraDataList;

// The record data of an enchantment.
struct EnchantmentItemData {
	// Byte swaps the first three dwords.
	void Endian();

	MagicSystem::CastingType eCastingType;
	int32_t iChargeOverride;
	int32_t iCostOverride;
	uint8_t iFlags;
};

// The enchantment of a weapon or apparel. Its own data members are not reconstructed yet.
class EnchantmentItem : public MagicItemForm {
public:
	// The PDB's flag bits of data.iFlags (COST_OVERRIDE: SetAutoCalc's push 1 at 0x409114; EXTEND_DURATION:
	// IsExtendDuration's GetFlag(2)). The PDB's third bit, HIDE_EFFECT, has no value the exe shows yet.
	enum EnchantmentFlag : int32_t {
		COST_OVERRIDE = 1,
		EXTEND_DURATION = 2,
	};

	EnchantmentItem();
	~EnchantmentItem() override;

	void InitializeData() override;
	void ClearData() override;
	bool IsAutoCalc() override;
	void SetAutoCalc(bool abValue) override;
	MagicSystem::SpellType GetSpellType() override { return MagicSystem::ENCHANTMENT; }
	float GetCost(Actor* apActor) override;

protected:
	void Endian() override;
	CHUNK_ID GetChunkID() override;

public:
	void* GetData() override { return &data; }

protected:
	uint32_t GetDataSize() override;
	bool CompareMagicItemData(MagicItem* apCompare) override;
	void CopyMagicItemData(MagicItem* apCopy) override;
	void SaveMagicItemData() override;

public:
	// whether a data flag is set.
	bool GetFlag(uint32_t aiValue);
	// Sets or clears the aiFlag bits of data.iFlags.
	void SetFlag(EnchantmentFlag aiFlag, bool abValue);
	// Whether a repeated cast restarts the effect instead of replacing it: GetFlag(2).
	bool IsExtendDuration();
	// The casting type (data.eCastingType). The body is folded with TES::GetInterior at 0x5F36F0.
	MagicSystem::CastingType GetCastingType();
	// Whether the enchantment can go on apOwner: armour and clothing take constant effects (unless an effect uses the
	// associated armour), scrolls cast once, weapons, ammo, NPCs and creatures take strikes, explosions any.
	bool IsValidEnchantForObject(TESForm* apOwner);

	// Whether apEffectSetting may enchant apEnchantableItem (with its extra data apExtra);
	// abFromMenus for the enchanting menus.
	static bool IsValidEffectForEnchantment(TESBoundObject* apEnchantableItem, ExtraDataList* apExtra,
		EffectSetting* apEffectSetting, bool abFromMenus);

	EnchantmentItemData data;
};
inline constexpr fnvd::Vtable EnchantmentItem_vftable_EffectItemList{};
inline constexpr fnvd::Vtable EnchantmentItem_vftable_MagicItem{};
inline constexpr fnvd::Vtable EnchantmentItem_vftable{};
