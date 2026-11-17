#pragma once

#include "Fallout Shared/Magic/MagicItemForm.h"

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

// The enchantment of a weapon or apparel. Its own data members are not declared.
class EnchantmentItem : public MagicItemForm {
public:
	// Flag bits of data.iFlags. A third bit, HIDE_EFFECT, is not declared because its value is not known.
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
	// The casting type (data.eCastingType).
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
