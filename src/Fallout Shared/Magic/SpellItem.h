#pragma once

#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/SkillLevel.h"

// The spell's DATA chunk.
struct SpellItemData {
	// Byte swaps the first three dwords.
	void Endian();

	MagicSystem::SpellType iSpellType;
	// Signed, as in AlchemyItemData and IngredientItemData: GetCost converts it as a signed value.
	int32_t iCostOverride;
	SkillLevel::Value eLevelOverride;
	uint8_t iFlags;
};

// A spell, disease, ability or addiction form.
class SpellItem : public MagicItemForm {
public:
	// Flag bits of data.iFlags (SetFlag uses the whole field).
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
	// Tests the whole byte; bool would assume 0/1.
	void SetFlag(SpellFlag aiFlag, bool abValue);

	SpellItemData data;
};
