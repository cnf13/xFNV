#pragma once

#include <cstddef>
#include <cstdint>

// The kinds of script command parameter (SCRIPT_PARAMETER::eParamType).
enum SCRIPT_PARAM_TYPE : uint32_t {
    SCRIPT_PARAM_CHAR = 0,
    SCRIPT_PARAM_INT = 1,
    SCRIPT_PARAM_FLOAT = 2,
    SCRIPT_PARAM_INVENTORY_OBJECT = 3,
    SCRIPT_PARAM_OBJECTREF = 4,
    SCRIPT_PARAM_ACTOR_VALUE = 5,
    SCRIPT_PARAM_ACTOR = 6,
    SCRIPT_PARAM_SPELL_ITEM = 7,
    SCRIPT_PARAM_AXIS = 8,
    SCRIPT_PARAM_CELL = 9,
    SCRIPT_PARAM_ANIM_GROUP = 10,
    SCRIPT_PARAM_MAGIC_ITEM = 11,
    SCRIPT_PARAM_SOUND = 12,
    SCRIPT_PARAM_TOPIC = 13,
    SCRIPT_PARAM_QUEST = 14,
    SCRIPT_PARAM_RACE = 15,
    SCRIPT_PARAM_CLASS = 16,
    SCRIPT_PARAM_FACTION = 17,
    SCRIPT_PARAM_SEX = 18,
    SCRIPT_PARAM_GLOBAL = 19,
    SCRIPT_PARAM_FURNITURE_OR_FORMLIST = 20,
    SCRIPT_PARAM_OBJECT = 21,
    SCRIPT_PARAM_SCRIPT_VAR = 22,
    SCRIPT_PARAM_STAGE = 23,
    SCRIPT_PARAM_MAP_MARKER = 24,
    SCRIPT_PARAM_ACTOR_BASE = 25,
    SCRIPT_PARAM_CONTAINER_REF = 26,
    SCRIPT_PARAM_WORLD = 27,
    SCRIPT_PARAM_CRIME_TYPE = 28,
    SCRIPT_PARAM_PACKAGE = 29,
    SCRIPT_PARAM_COMBAT_STYLE = 30,
    SCRIPT_PARAM_MAGIC_EFFECT = 31,
    SCRIPT_PARAM_FORM_TYPE = 32,
    SCRIPT_PARAM_WEATHER = 33,
    SCRIPT_PARAM_NPC = 34,
    SCRIPT_PARAM_OWNER = 35,
    SCRIPT_PARAM_SHADER_EFFECT = 36,
    SCRIPT_PARAM_FORMLIST = 37,
    SCRIPT_PARAM_MENUICON = 38,
    SCRIPT_PARAM_PERK = 39,
    SCRIPT_PARAM_NOTE = 40,
    SCRIPT_PARAM_MISC_STAT = 41,
    SCRIPT_PARAM_IMAGESPACEMOD = 42,
    SCRIPT_PARAM_IMAGESPACE = 43,
    SCRIPT_PARAM_VATS_VALUE = 44,
    SCRIPT_PARAM_VATS_VALUE_DATA = 45,
    SCRIPT_PARAM_VOICE_TYPE = 46,
    SCRIPT_PARAM_ENCOUNTERZONE = 47,
    SCRIPT_PARAM_IDLE_FORM = 48,
    SCRIPT_PARAM_MESSAGE = 49,
    SCRIPT_PARAM_INVOBJECT_OR_FORMLIST = 50,
    SCRIPT_PARAM_ALIGNMENT = 51,
    SCRIPT_PARAM_EQUIPTYPE = 52,
    SCRIPT_PARAM_OBJECT_OR_FORMLIST = 53,
    SCRIPT_PARAM_MUSIC = 54,
    SCRIPT_PARAM_CRITSTAGE = 55,
    SCRIPT_PARAM_NPC_OR_LEVCHAR = 56,
    SCRIPT_PARAM_CREA_OR_LEVCREA = 57,
    SCRIPT_PARAM_LEVCHAR = 58,
    SCRIPT_PARAM_LEVCREA = 59,
    SCRIPT_PARAM_LEVITEM = 60,
    SCRIPT_PARAM_FORM = 61,
    SCRIPT_PARAM_REPUTATION = 62,
    SCRIPT_PARAM_CASINO = 63,
    SCRIPT_PARAM_CASINOCHIP = 64,
    SCRIPT_PARAM_CHALLENGE = 65,
    SCRIPT_PARAM_CARAVANMONEY = 66,
    SCRIPT_PARAM_CARAVANCARD = 67,
    SCRIPT_PARAM_CARAVANDECK = 68,
    SCRIPT_PARAM_REGION = 69,
    SCRIPT_PARAM_COUNT = 70,
};

// One parameter of a script command: its name, its SCRIPT_PARAM_TYPE and whether it may be left out. The commands'
// parameter tables are arrays of these.
struct SCRIPT_PARAMETER {
    const char* pParamName;
    SCRIPT_PARAM_TYPE eParamType;
    bool bOptional;
    uint8_t padding09[3];
};

// A command's parameter table (the tables are TESScriptShared.cpp's, each sized by its parameters).
template <size_t N>
using ScriptParameterArray = SCRIPT_PARAMETER[N];

// How each parameter type is compiled and parsed: whether it may name a variable and whether it is a referenced
// object (a form reached through the script's reference list).
struct SCRIPT_PARAMETER_DEF {
    SCRIPT_PARAM_TYPE eParamType;
    bool bCanBeVariable;
    bool bReferencedObject;
    uint8_t padding06[2];
};

// The definition of each parameter type, indexed by SCRIPT_PARAM_TYPE.
using ScriptParameterDefArray = SCRIPT_PARAMETER_DEF[SCRIPT_PARAM_COUNT];
inline ScriptParameterDefArray scriptParameterDefs{
    {},
    {SCRIPT_PARAM_INT, true},
    {SCRIPT_PARAM_FLOAT, true},
    {SCRIPT_PARAM_INVENTORY_OBJECT, false, true},
    {SCRIPT_PARAM_OBJECTREF, false, true},
    {SCRIPT_PARAM_ACTOR_VALUE},
    {SCRIPT_PARAM_ACTOR, false, true},
    {SCRIPT_PARAM_SPELL_ITEM, false, true},
    {SCRIPT_PARAM_AXIS},
    {SCRIPT_PARAM_CELL, false, true},
    {SCRIPT_PARAM_ANIM_GROUP},
    {SCRIPT_PARAM_MAGIC_ITEM, false, true},
    {SCRIPT_PARAM_SOUND, false, true},
    {SCRIPT_PARAM_TOPIC, false, true},
    {SCRIPT_PARAM_QUEST, false, true},
    {SCRIPT_PARAM_RACE, false, true},
    {SCRIPT_PARAM_CLASS, false, true},
    {SCRIPT_PARAM_FACTION, false, true},
    {SCRIPT_PARAM_SEX},
    {SCRIPT_PARAM_GLOBAL, false, true},
    {SCRIPT_PARAM_FURNITURE_OR_FORMLIST, false, true},
    {SCRIPT_PARAM_OBJECT, false, true},
    {SCRIPT_PARAM_SCRIPT_VAR},
    {SCRIPT_PARAM_STAGE, true},
    {SCRIPT_PARAM_MAP_MARKER, false, true},
    {SCRIPT_PARAM_ACTOR_BASE, false, true},
    {SCRIPT_PARAM_CONTAINER_REF, false, true},
    {SCRIPT_PARAM_WORLD, false, true},
    {SCRIPT_PARAM_CRIME_TYPE},
    {SCRIPT_PARAM_PACKAGE, false, true},
    {SCRIPT_PARAM_COMBAT_STYLE, false, true},
    {SCRIPT_PARAM_MAGIC_EFFECT, false, true},
    {SCRIPT_PARAM_FORM_TYPE},
    {SCRIPT_PARAM_WEATHER, false, true},
    {SCRIPT_PARAM_NPC, false, true},
    {SCRIPT_PARAM_OWNER, false, true},
    {SCRIPT_PARAM_SHADER_EFFECT, false, true},
    {SCRIPT_PARAM_FORMLIST, false, true},
    {SCRIPT_PARAM_MENUICON, false, true},
    {SCRIPT_PARAM_PERK, false, true},
    {SCRIPT_PARAM_NOTE, false, true},
    {SCRIPT_PARAM_MISC_STAT},
    {SCRIPT_PARAM_IMAGESPACEMOD, false, true},
    {SCRIPT_PARAM_IMAGESPACE, false, true},
    {SCRIPT_PARAM_VATS_VALUE, false, true},
    {SCRIPT_PARAM_VATS_VALUE_DATA},
    {SCRIPT_PARAM_VOICE_TYPE, false, true},
    {SCRIPT_PARAM_ENCOUNTERZONE, false, true},
    {SCRIPT_PARAM_IDLE_FORM, false, true},
    {SCRIPT_PARAM_MESSAGE, false, true},
    {SCRIPT_PARAM_INVOBJECT_OR_FORMLIST, false, true},
    {SCRIPT_PARAM_ALIGNMENT},
    {SCRIPT_PARAM_EQUIPTYPE},
    {SCRIPT_PARAM_OBJECT_OR_FORMLIST, false, true},
    {SCRIPT_PARAM_MUSIC, false, true},
    {SCRIPT_PARAM_CRITSTAGE},
    {SCRIPT_PARAM_NPC_OR_LEVCHAR, false, true},
    {SCRIPT_PARAM_CREA_OR_LEVCREA, false, true},
    {SCRIPT_PARAM_LEVCHAR, false, true},
    {SCRIPT_PARAM_LEVCREA, false, true},
    {SCRIPT_PARAM_LEVITEM, false, true},
    {SCRIPT_PARAM_FORM, false, true},
    {SCRIPT_PARAM_REPUTATION, false, true},
    {SCRIPT_PARAM_CASINO, false, true},
    {SCRIPT_PARAM_CASINOCHIP, false, true},
    {SCRIPT_PARAM_CHALLENGE, false, true},
    {SCRIPT_PARAM_CARAVANMONEY, false, true},
    {SCRIPT_PARAM_CARAVANCARD, false, true},
    {SCRIPT_PARAM_CARAVANDECK, false, true},
    {SCRIPT_PARAM_REGION, false, true}
};
