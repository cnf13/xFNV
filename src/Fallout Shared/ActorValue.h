#pragma once

#include "Fallout Shared/TESFile.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class ActorValueInfo;
class ActorValueOwner;
class Menu;
class Tile;

// The actor value table. Every actor value has one shared entry in ActorValue::Instance; the static
// helpers convert between a value's index, the section it belongs to and its flagged behaviour.
class ActorValue {
public:
    // The fixed index of every actor value, in the order of the engine's own table.
    enum Index : int32_t {
        AGGRESSION = 0x0,
        CONFIDENCE = 0x1,
        ENERGY = 0x2,
        RESPONSIBILITY = 0x3,
        MOOD = 0x4,
        STRENGTH = 0x5,
        PERCEPTION = 0x6,
        ENDURANCE = 0x7,
        CHARISMA = 0x8,
        INTELLIGENCE = 0x9,
        AGILITY = 0xA,
        LUCK = 0xB,
        ACTION_POINTS = 0xC,
        CARRY_WEIGHT = 0xD,
        CRITICAL_CHANCE = 0xE,
        HEALING_RATE = 0xF,
        HEALTH = 0x10,
        MELEE_DAMAGE = 0x11,
        DAMAGE_RESISTANCE = 0x12,
        POISON_RESISTANCE = 0x13,
        RADIATION_RESISTANCE = 0x14,
        SPEED_MULT = 0x15,
        FATIGUE = 0x16,
        KARMA = 0x17,
        EXPERIENCE_POINTS = 0x18,
        PERCEPTION_CONDITION = 0x19,
        ENDURANCE_CONDITION = 0x1A,
        LEFT_ATTACK_CONDITION = 0x1B,
        RIGHT_ATTACK_CONDITION = 0x1C,
        LEFT_MOBILITY_CONDITION = 0x1D,
        RIGHT_MOBILITY_CONDITION = 0x1E,
        BRAIN_CONDITION = 0x1F,
        BARTER = 0x20,
        BIG_GUNS = 0x21,
        ENERGY_WEAPONS = 0x22,
        EXPLOSIVES = 0x23,
        LOCKPICK = 0x24,
        MEDICINE = 0x25,
        MELEE_WEAPONS = 0x26,
        REPAIR = 0x27,
        SCIENCE = 0x28,
        SMALL_GUNS = 0x29,
        SNEAK = 0x2A,
        SPEECH = 0x2B,
        SURVIVAL = 0x2C,
        UNARMED = 0x2D,
        INVENTORY_WEIGHT = 0x2E,
        PARALYSIS = 0x2F,
        INVISIBILITY = 0x30,
        CHAMELEON = 0x31,
        NIGHT_EYE_BONUS = 0x32,
        TURBO = 0x33,
        FIRE_RESISTANCE = 0x34,
        WATER_BREATHING = 0x35,
        RADIATION_RADS = 0x36,
        BLOODY_MESS = 0x37,
        UNARMED_DAMAGE = 0x38,
        ASSISTANCE = 0x39,
        ELECTRICAL_RESISTANCE = 0x3A,
        FROST_RESISTANCE = 0x3B,
        ENERGY_RESISTANCE = 0x3C,
        EMP_RESISTANCE = 0x3D,
        VARIABLE_01 = 0x3E,
        VARIABLE_02 = 0x3F,
        VARIABLE_03 = 0x40,
        VARIABLE_04 = 0x41,
        VARIABLE_05 = 0x42,
        VARIABLE_06 = 0x43,
        VARIABLE_07 = 0x44,
        VARIABLE_08 = 0x45,
        VARIABLE_09 = 0x46,
        VARIABLE_10 = 0x47,
        IGNORE_CRIPPLED_LIMBS = 0x48,
        DEHYDRATION = 0x49,
        HUNGER = 0x4A,
        SLEEPDEPREVATION = 0x4B,
        DAMAGE_THRESHOLD = 0x4C,
        ACTOR_VALUE_COUNT = 0x4D,
        AI_ATTRIBUTE_START = 0x0,
        ATTRIBUTE_START = 0x5,
        ACTOR_VALUE_START = 0x0,
        DERIVED_ATTRIBUTE_START = 0xC,
        CONDITION_START = 0x19,
        SKILL_START = 0x20,
        INT_START = 0x2E,
        VARIABLE_START = 0x3E,
        AI_ATTRIBUTE_END = 0x5,
        ATTRIBUTE_END = 0xC,
        CONDITION_END = 0x20,
        DERIVED_ATTRIBUTE_END = 0x20,
        SKILL_END = 0x2E,
        INT_END = 0x3E,
        VARIABLE_END = 0x4C,
        ATTRIBUTE_COUNT = 0x7,
        DERIVED_ATTRIBUTE_COUNT = 0x14,
        SKILL_COUNT = 0xE,
        AI_ATTRIBUTE_COUNT = 0x5,
        CONDITION_COUNT = 0x7,
        INT_COUNT = 0x10,
        VALUE_NONE = -1,
        SKILL_NONE = -1,
        ATTRIBUTE_NONE = -1,
    };

    // The behaviour flags of an actor value's table entry.
    enum Flags : uint32_t {
        FAST_MODIFIER = 0x1,
        CREATURE_MAPPED = 0x2,
        EFFECT_SKILL = 0x4,
        MAX_TEN = 0x8,
        MAX_ONE_HUNDRED = 0x10,
        DAMAGE_MIN_ZERO = 0x20,
        DERIVED_PLUS_BASE = 0x40,
        DERIVED_IF_AUTOCALC = 0x80,
        ENUMERATION = 0x100,
        DAMAGE_IS_POSITIVE = 0x200,
        COMBAT_SKILL = 0x400,
        DERIVED_ALWAYS = 0x800,
        CREATURE_NOT_DERIVED = 0x1000,
        DEPRECATED = 0x2000,
        NO_SCRIPT_MODAV = 0x4000,
        MIN_ONE = 0x8000,
    };

    // The kind of an actor value (ActorValueInfo::eAVType); GetSectionCount counts the values of each.
    enum Section : int32_t {
        ATTRIBUTE = 0x0,
        DERIVED_ATTRIBUTE = 0x1,
        SKILL = 0x2,
        AI_ATTRIBUTE = 0x3,
        CONDITION = 0x4,
        INT_VALUE = 0x5,
        RESISTANCE = 0x6,
        VARIABLE = 0x7,
        SECTION_COUNT = 0x8,
    };

    ActorValue();
    ~ActorValue();

    static ActorValueInfo* GetActorValueInfo(Index aiIndex);
    // The value's abbreviation (its table entry's name), or null.
    static const char* GetActorValueAbbreviation(Index aiIndex);
    // The values that depend on aiWhich, with their count in aiCount; null and 0 without an info.
    static Index* GetDependentActorValues(Index aiWhich, int32_t& aiCount);
    // Whether aiWhich has a modified callback.
    static bool HasModifiedCallback(Index aiWhich);
    // Calls aiWhich's modified callback, if any, on apActor; afDelta is the new value at the
    // call sites seen.
    static void CheckCallModifiedCallback(
        ActorValueOwner* apActor,
        Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );
    static bool IsSkill(Index aiIndex);
    static bool IsSPECIAL(Index aiIndex);
    static bool IsDerivedAttribute(Index aiIndex);
    static bool IsAIAttribute(Index aiIndex);
    static bool IsFlagSet(Index aiIndex, Flags aiFlag);
    static char ToArrayIndex(ActorValue::Section aiSection, Index aiIndex);
    static Index ToActorValue(ActorValue::Section aiSection, char aiIndex);
    static ActorValue::Section GetSection(Index aiWhich);
    static float ClampActorValue(Index aiWhich, float afValue);
    // The number of actor values in section aiSection; 0 for an unknown section.
    static uint32_t GetSectionCount(ActorValue::Section aiSection);
    // The value's display name (its table entry's full name).
    static const char* GetActorValueName(Index aiIndex);
    // The value's script name.
    static const char* GetActorValueScriptName(Index aiIndex);
    // The value's description.
    static const char* GetActorValueDesc(Index aiIndex);
    // The value's icon path.
    static const char* GetActorValueIcon(Index aiIndex);
    // The weapon type badge of a weapon skill, else null.
    static const char* GetItemWeaponTypeBadge(Index aiIndex);
    // Fills the actor value table.
    static void RegisterActorValues();
    // Creates aiWhich's table entry, if it has none, from these values; the aiNumDependentActorValues values aiWhich
    // derives (up to 15 are kept) follow as Index arguments. aeAVType and apModifiedCallback have the types of
    // ActorValueInfo::eAVType and pModifiedCallback, where they are stored; auFlags is an OR of Flags bits, stored in
    // the uint32_t uFlags.
    static void RegisterActorValue(
        Index aiWhich,
        const char* asScriptName,
        Section aeAVType,
        uint32_t auFlags,
        float (*apDerivationFunction)(ActorValueOwner*, Index),
        Index (*apMappingFunction)(ActorValueOwner*, Index),
        void (*apModifiedCallback)(ActorValueOwner*, Index, float, float, ActorValueOwner*),
        int32_t aiNumDependentActorValues,
        ...
    );
    // The value of aiWhich derived from apActor's other values into afValue; false when
    // aiWhich is not derived.
    static bool GetDerivedActorValue(ActorValueOwner* apActor, Index aiWhich, float& afValue);
    static ActorValue::Index GetActorValueIndex(char* acScriptName);
    static bool IsResistance(ActorValue::Index aiIndex);
    // Comparator for ListBox<Index>::Sort, passed by CharGenMenu::SetupSkillPoints; compares each item's data.
    static int32_t CompareForSorting(ListBoxItem<ActorValue::Index>* apItemA, ListBoxItem<ActorValue::Index>* apItemB);
    static void FreeActorValues();
    // The experience points' modified callback (RegisterActorValue stores it); it uses aiWhich and afDelta.
    static void HandleXPChange(
        ActorValueOwner* apActor,
        Index aiWhich,
        float afOriginalValue,
        float afDelta,
        ActorValueOwner* apActorSource
    );

    // The table of the value entries, by Index. The constructor zeroes it.
    ActorValueInfo* pActorValueInfo[ACTOR_VALUE_COUNT];

    // The actor value table; the entries are shared by every form that uses actor values.
    static ActorValue Instance;
};

inline bool bActorValuesRegistered;
inline int32_t ActorValueListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t aiTrait;
inline int32_t ActorValueListBox_IsSelected_iUT_Selected;
inline int32_t ActorValueListBox_SetSelection_iUT_Selected;
inline int32_t aeTrait;
inline int32_t ActorValueListBox_NumUnfiltered_iUT_num_filtered;
