#pragma once

#include "Fallout Shared/ActorValue.h"

class ActiveEffect;
class EffectItem;
class MagicCaster;
class MagicItem;

// The table of what each kind of magic effect (archetype) is called, what it can do and which active effect class runs
// it.
class EffectArchetypes {
public:
    // The archetypes, by EffectSetting's eArchetype. Values are counted from VALUE_MODIFIER = 0.
    enum ArchetypeID : int32_t {
        ARCHETYPE_NONE = -1,
        ARCHETYPE_START = 0,
        VALUE_MODIFIER = 0,
        SCRIPT = 1,
        DISPEL = 2,
        CURE_DISEASE = 3,
        ABSORB = 4,
        SHIELD = 5,
        CALM = 6,
        DEMORALIZE = 7,
        FRENZY = 8,
        COMMAND_CREATURE = 9,
        COMMAND_HUMANOID = 10,
        INVISIBILITY = 11,
        CHAMELEON = 12,
        LIGHT = 13,
        DARKNESS = 14,
        NIGHT_EYE = 15,
        LOCK = 16,
        OPEN = 17,
        BOUND_ITEM = 18,
        SUMMON_CREATURE = 19,
        DETECT_LIFE = 20,
        TELEKINESIS = 21,
        DISINTEGRATE_ARMOR = 22,
        DISINTEGRATE_WEAPON = 23,
        PARALYZE = 24,
        REANIMATE = 25,
        SOUL_TRAP = 26,
        TURN_UNDEAD = 27,
        SUN_DAMAGE = 28,
        VAMPIRISM = 29,
        CURE_PARALYSIS = 30,
        CURE_ADDICTION = 31,
        CURE_POISON = 32,
        CONCUSSION = 33,
        VALUE_AND_PARTS = 34,
        LIMB_CONDITION = 35,
        TURBO = 36,
        ARCHETYPE_COUNT = 37,
    };

    enum ARCHETYPE_FLAGS : uint32_t {
        USES_ASSOCIATED_ACTOR_VALUE = 0x2,
        USES_SCRIPT = 0x40,
        // Every bit whose archetype takes an associated item (USES_SCRIPT among them).
        ASSOCIATED_ITEM_MASK = 0x7C,
    };

    using InstantiateFunction = ActiveEffect* (*)(MagicCaster*, MagicItem*, EffectItem*);

    // One row of the table, by archetype (EffectSetting's eArchetype, 0 to 0x24).
    struct ArchetypeDef {
        const char* pcName;
        // The constructor an active effect class registers with RegisterActiveEffect, called by ActiveEffect::Activate.
        InstantiateFunction pInstantiateFunction;
        uint32_t iFlags;
        // The actor value the archetype fixes; -1 for none.
        ActorValue::Index iFixedActorValue;
    };

    using ArchetypeTable = ArchetypeDef[ARCHETYPE_COUNT];

    // Whether the archetype aiArchetype has a flag of aiFlags in its table row.
    static bool CheckFlag(int32_t aiArchetype, uint32_t aiFlags);
    // The name of archetype aiArchetype in the table.
    static const char* GetName(int32_t aiArchetype);
    // Sets archetype aeArchtype's instantiate function unless it has one; 1 when set.
    static char RegisterActiveEffect(ArchetypeID aeArchtype, InstantiateFunction apInstantiateFunction);

    // The instantiate functions are null in the image: each active effect class registers its own at start-up
    // (RegisterActiveEffect).
    static inline ArchetypeTable Archetypes{
        {"Value Modifier", nullptr, 2, ActorValue::VALUE_NONE},
        {"Script", nullptr, 64, ActorValue::VALUE_NONE},
        {"Dispel", nullptr, 0, ActorValue::VALUE_NONE},
        {"Cure Disease", nullptr, 0, ActorValue::VALUE_NONE},
        {"Absorb", nullptr, 3, ActorValue::VALUE_NONE},
        {"Shield", nullptr, 1, ActorValue::VALUE_NONE},
        {"Calm", nullptr, 3, ActorValue::AGGRESSION},
        {"Demoralize", nullptr, 3, ActorValue::CONFIDENCE},
        {"Frenzy", nullptr, 3, ActorValue::AGGRESSION},
        {"Command Creature", nullptr, 1, ActorValue::VALUE_NONE},
        {"Command Humanoid", nullptr, 1, ActorValue::VALUE_NONE},
        {"Invisibility", nullptr, 2, ActorValue::INVISIBILITY},
        {"Chameleon", nullptr, 2, ActorValue::CHAMELEON},
        {"Light", nullptr, 0, ActorValue::VALUE_NONE},
        {"Darkness", nullptr, 1, ActorValue::VALUE_NONE},
        {"Night Eye", nullptr, 3, ActorValue::NIGHT_EYE_BONUS},
        {"Lock", nullptr, 0, ActorValue::VALUE_NONE},
        {"Open", nullptr, 0, ActorValue::VALUE_NONE},
        {"Bound Item", nullptr, 48, ActorValue::VALUE_NONE},
        {"Summon Creature", nullptr, 12, ActorValue::VALUE_NONE},
        {"Detect Life", nullptr, 1, ActorValue::VALUE_NONE},
        {"Telekinesis", nullptr, 1, ActorValue::VALUE_NONE},
        {"Disintigrate Armor", nullptr, 1, ActorValue::VALUE_NONE},
        {"Disinitgrate Weapon", nullptr, 1, ActorValue::VALUE_NONE},
        {"Paralysis", nullptr, 2, ActorValue::PARALYSIS},
        {"Reanimate", nullptr, 1, ActorValue::VALUE_NONE},
        {"Soul Trap", nullptr, 1, ActorValue::VALUE_NONE},
        {"Turn Undead", nullptr, 1, ActorValue::VALUE_NONE},
        {"Sun Damage", nullptr, 1, ActorValue::VALUE_NONE},
        {"Vampirism", nullptr, 1, ActorValue::VALUE_NONE},
        {"Cure Paralysis", nullptr, 0, ActorValue::VALUE_NONE},
        {"Cure Addiction", nullptr, 0, ActorValue::VALUE_NONE},
        {"Cure Poison", nullptr, 0, ActorValue::VALUE_NONE},
        {"Concussion", nullptr, 0, ActorValue::VALUE_NONE},
        {"Value and Parts", nullptr, 2, ActorValue::VALUE_NONE},
        {"Limb Condition", nullptr, 0, ActorValue::VALUE_NONE},
        {"Turbo", nullptr, 2, ActorValue::TURBO}
    };
};
