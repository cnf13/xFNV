#pragma once

#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/SkillLevel.h"
#include "Reconstruction.hpp"

// The spell's DATA chunk (the IDA name is SpellItem::Data).
struct SpellItemData {
	// Byte swaps the first three dwords.
	void Endian();

	MagicSystem::SpellType iSpellType;
	// Signed: GetCost converts it with fild, as AlchemyItemData's and IngredientItemData's.
	int32_t iCostOverride;
	SkillLevel::Value eLevelOverride;
	uint8_t iFlags;
};

// A spell, disease, ability or addiction form.
class SpellItem : public MagicItemForm {
public:
	// The PDB's flag bits of data.iFlags (SetFlag uses the whole dword: 0x40E248).
	enum SpellFlag : int32_t {
		COST_OVERRIDE = 1,
		ALWAYS_SUCCEEDS = 2,
		PC_STARTSPELL = 4,
		IMMUNE_TO_SILENCE = 8,
		IGNORE_LOS_CHECK = 16,
		IGNORE_RESISTANCE = 32,
		NO_ABSORB_REFLECT = 64,
		FORCE_TOUCH_EXPLODE = 128,
	};

	SpellItem();
	~SpellItem() override;

	void InitializeData() override;
	void ClearData() override;
	bool IsAutoCalc() override;
	void SetAutoCalc(bool abValue) override;
	MagicSystem::SpellType GetSpellType() override { return data.iSpellType; }
	bool IgnoreResistance() override;
	bool GetNoAbsorbOrReflect() override;
	float GetCost(Actor* apActor) override;
	SkillLevel::Value GetLevel() override;

protected:
	void Endian() override;
	CHUNK_ID GetChunkID() override;
	void* GetData() override { return &data; }
	uint32_t GetDataSize() override;
	bool CompareMagicItemData(MagicItem* apCompare) override;
	void CopyMagicItemData(MagicItem* apCopy) override;
	void SaveMagicItemData() override;

public:
	bool GetFlag(uint32_t aiValue);
	bool IsPCStartSpell();
	// The exe tests the whole byte; bool would assume 0/1.
	void SetFlag(SpellFlag aiFlag, bool abValue);

	SpellItemData data;
};
inline constexpr fnvd::Vtable SpellItem_vftable_EffectItemList{};
inline constexpr fnvd::Vtable SpellItem_vftable_MagicItem{};
inline constexpr fnvd::Vtable SpellItem_vftable{};
