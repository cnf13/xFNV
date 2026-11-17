#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Reconstruction.hpp"

class Actor;
class EffectSetting;
class Script;

// Inferred type: a1: float from 0x40404d (parameter 0 of FloatFloor)
double fFloor(float a1);
// The game's floor of a float (the PDB's FloatFloor(float aIn) in EffectItem.cpp; IDA: floor), 0x404040 (cdecl): the
// truncation fFloor 0x404090 gives, less 1 when aIn lies below it; the result comes back in ST(0) rounded to float.
// Not the CRT's floor: it never calls _floor.
float FloatFloor(float aIn);
MEM_CONTEXT QMemContext();

// The numbers of one magic effect entry.
struct EffectItemData {
	EffectItemData();

	// Swaps the five dwords read from a big-endian file.
	void Endian();

	int32_t iMagnitude;
	int32_t iArea;
	int32_t iDuration;
	MagicSystem::Range iRange;
	ActorValue::Index iActorValue;
};

// One effect of a magic item: its numbers, the magic effect form, its raw cost and its conditions.
class EffectItem {
public:
	EffectItem(EffectSetting* apEffectSetting);
	EffectItem(EffectItem* a1);

	// The magnitude, 0 when the effect has no magnitude.
	int32_t GetMagnitude();
	// The duration, 0 when the effect has no duration.
	int32_t GetDuration();
	// The area, 0 when the effect has no area.
	int32_t GetArea();
	// The range; the exe's folded +0xC getter (thiscall, ret 0, 0x84E3A0).
	MagicSystem::Range GetRange();
	// Each setter stores a value the magic effect allows and resets the cached raw cost; true when it was stored.
	bool SetMagnitude(int32_t aiMagnitude);
	bool SetArea(int32_t aiArea);
	bool SetDuration(int32_t aiDuration);
	bool SetRange(MagicSystem::Range aiRange);
	// True when the effect is the usage monitor effect.
	bool IsUsageMonitorEffect();
	// True when the effect's archetype uses its actor value.
	bool IsActorValueUsed();
	ActorValue::Index GetActorValueIndex();
	void SetActorValueIndex(ActorValue::Index aiIndex) { data.iActorValue = aiIndex; }
	// True when the effect's magic effect is hostile.
	bool IsHostile();
	// The effect's magic-effect form (shared getter at 0x825C00).
	EffectSetting* GetEffectSetting();
	// The exe leaves the field's value in ST0 and its callers round it to a float.
	float GetRawCost();
	bool IsDisabled();
	// The skill of the effect's magic school.
	// Inferred type: return: ActorValue::Index: passed to ActorValueOwner::GetClampedActorValue 0 at 0x403fb3
	ActorValue::Index GetMagicSchool();
	// The raw cost scaled by apActor's school skill and luck when there is a caster, floored, at least 1.
	float GetCost(Actor* apActor);
	bool IsScriptEffect();
	Script* GetScript();
	// Builds the effect's name in apResult, the exe's hidden result (thiscall, ret 4, 0x404670);
	// returns apResult.
	BSStringT<char>* GetEffectName(BSStringT<char>* apString);
	void Initialize(EffectSetting* apEffectSetting);
	// Inferred type: a2: TESForm* from 0x40c084 (parameter 0 of MagicItem::MagicItemInitItem)
	void InitItem(TESForm* a2);
	// Saves the effect's numbers (EFIT) and its conditions.
	void Save();
	bool Load(TESFile* apFile, const char* apSpellName);
	void Copy(EffectItem* apCopy);
	bool MatchesEffect(EffectItem* apCompare);
	void SetConditions(TESCondition* apCondition);

	// Whether both effects use the associated
	// armour (of matching biped slots) or both the associated weapon.
	static bool SharesAssociatedItem(EffectItem* apEffect1, EffectItem* apEffect2);
	static void operator delete(void* apMemory);

	EffectItemData data;
	EffectSetting* pEffectSetting;
	float fRawCost;
	TESCondition Conditions;
};

// The game settings EffectItem.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> sMagicEffectItemPointsPlural;
