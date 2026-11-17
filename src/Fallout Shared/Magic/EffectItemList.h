#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/SkillLevel.h"

class Actor;
class EffectItem;
class Script;

int32_t BSvsprintf(char* apString1, const char* apFormat, va_list aList);

// The effects of a magic item, with the count of hostile ones. The destructor is deliberately not virtual, as in the
// game.
class EffectItemList : public BSSimpleList<EffectItem*> {
public:
	EffectItemList();
	~EffectItemList();

	virtual bool IsMedicine() { return false; }
	virtual bool IsFood() { return false; }
	virtual float GetCost(Actor* apActor);
	virtual int32_t GetMaxEffectCount() { return 0; }
	virtual SkillLevel::Value GetLevel() { return SkillLevel::SKILL_LEVEL_COUNT; }

	// Writes the effects' description into the auiSize-byte buffer apBuffer.
	void GetEffectsString(char* apBuffer, uint32_t auiSize);
	// Whether any effect has a flag of aeFlag.
	bool HasEffect(EffectSetting::EffectFlag aeFlag);
	// Whether any effect has the self range.
	bool HasSelfEffect();
	// Whether any effect has the touch range.
	bool HasTouchEffect();
	// Saves each effect as its EFID chunk and the effect's own chunks.
	void Save();
	// Reads the effect chunks of apFile into the list; apSpellName is the item's name.
	void Load(TESFile* apFile, const char* apSpellName);
	// True when the list has hostile effects (iHostileCount > 0, signed).
	bool IsHostile();
	// Whether the item can be used as a poison.
	bool CanBePoison();
	void RemoveAllEffects();
	// The usage monitor effect of the list, or null.
	EffectItem* GetUsageMonitorEffect();
	EffectItem* GetCostliestEffect(MagicSystem::Range aeRange, bool abAreaOnly);
	bool HasTargetEffect();
	ActorValue::Index GetAssociatedSkill();
	// The summed score of the effects of archetype aeArchetypeID and range aeRange on actor value aiActorValue.
	float GetEffectScore(EffectArchetypes::ArchetypeID aeArchetypeID, MagicSystem::Range aeRange,
		ActorValue::Index aiActorValue);
	void AddEffect(EffectItem* apEffect);
	void Copy(EffectItemList* apCopy);
	EffectItem* GetEffectItemByIndex(uint8_t aiIndex);
	bool IsDisabled();
	// True when the lists differ.
	bool Compare(EffectItemList* apCompare);
	uint8_t GetEffectItemIndex(EffectItem* apEffect);
	// Whether an enabled effect runs apScript.
	bool HasScript(Script* apScript);

	uint32_t iHostileCount;
};

inline EffectItem* pLastEffect;
