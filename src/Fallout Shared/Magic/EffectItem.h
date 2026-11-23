#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESCondition.h"

class Actor;
class EffectSetting;
class Script;

double fFloor(float afValue);
// The game's floor of a float: the truncation fFloor gives, less 1 when aIn lies below it, with the result rounded to
// float. Not the CRT's floor.
float FloatFloor(float aIn);

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
    EffectItem(EffectItem* apCopy);

    // The magnitude, 0 when the effect has no magnitude.
    int32_t GetMagnitude();
    // The duration, 0 when the effect has no duration.
    int32_t GetDuration();
    // The area, 0 when the effect has no area.
    int32_t GetArea();
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
    void SetActorValueIndex(ActorValue::Index aiIndex) {
        data.iActorValue = aiIndex;
    }
    // True when the effect's magic effect is hostile.
    bool IsHostile();
    EffectSetting* GetEffectSetting();
    float GetRawCost();
    bool IsDisabled();
    // The skill of the effect's magic school.
    ActorValue::Index GetMagicSchool();
    // The raw cost scaled by apActor's school skill and luck when there is a caster, floored, at least 1.
    float GetCost(Actor* apActor);
    bool IsScriptEffect();
    Script* GetScript();
    // Builds the effect's name in apString; returns apString.
    BSStringT<char> GetEffectName();
    void Initialize(EffectSetting* apEffectSetting);
    void InitItem(TESForm* apOwner);
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

// The game settings EffectItem.cpp defines.
extern SettingT<GameSettingCollection> sMagicEffectItemPointsPlural;
