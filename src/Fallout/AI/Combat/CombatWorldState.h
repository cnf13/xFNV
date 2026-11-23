#pragma once

#include "BSCore/BSSimpleList.h"

class CombatController;

// The facts of the world a combat plan tracks.
enum COMBAT_WORLD_STATE : int32_t {
    COMBAT_WORLD_STATE_TARGET_DEAD = 0x0,
    COMBAT_WORLD_STATE_AT_LOCATION = 0x1,
    COMBAT_WORLD_STATE_NOT_AT_LOCATION = 0x2,
    COMBAT_WORLD_STATE_TARGET_LOST = 0x3,
    COMBAT_WORLD_STATE_TARGET_DETECTED = 0x4,
    COMBAT_WORLD_STATE_TARGET_360_LOS = 0x5,
    COMBAT_WORLD_STATE_TARGET_BLOCKED = 0x6,
    COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT = 0x7,
    COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED = 0x8,
    COMBAT_WORLD_STATE_TARGET_FLUSHED = 0x9,
    COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN = 0xA,
    COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED = 0xB,
    COMBAT_WORLD_STATE_HAS_WEAPON_TYPE = 0xC,
    COMBAT_WORLD_STATE_HAS_COMBAT_ITEM = 0xD,
    COMBAT_WORLD_STATE_THREATENED = 0xE,
    COMBAT_WORLD_STATE_WANTS_TO_FLEE = 0xF,
    COMBAT_WORLD_STATE_USED_COMBAT_ITEM = 0x10,
    COMBAT_WORLD_STATE_HIDING = 0x11,
    COMBAT_WORLD_STATE_EXECUTION_FLAGS = 0x12,
    COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT = 0x13,
    COMBAT_WORLD_STATE_COUNT = 0x14,
    // Goal conditions on a specified variable, tested by CombatGoal::IsActive.
    SPECIFIED_VARIABLE_ZERO = 0x15,
    SPECIFIED_VARIABLE_NON_ZERO = 0x16,
};

// The combat locations, combat items and specified variables a plan step names.
enum COMBAT_VARIABLE : int32_t {
    TARGET_LOCATION = 0,
    NEAR_TARGET_LOCATION = 1,
    COVER_LOCATION = 2,
    DODGE_LOCATION = 3,
    FLANK_LOCATION = 4,
    FALLBACK_LOCATION = 5,
    ADVANCE_LOCATION = 6,
    DESTINATION_LOCATION = 7,
    GUARD_LOCATION = 8,
    CLUSTER_LOCATION = 9,
    HIDE_LOCATION = 10,
    GRENADE_LOCATION = 11,
    FLEE_LOCATION = 12,
    LOS_LOCATION = 13,
    SAFE_LOCATION = 14,
    INVESTIGATE_LOCATION = 15,
    COMBAT_AREA_LOCATION = 16,
    COMBAT_LOCATION_COUNT = 17,
    COMBAT_ITEM_START = 17,
    COMBAT_ITEM_RESTORE = 17,
    COMBAT_ITEM_BUFF = 18,
    COMBAT_ITEM_USABLE_END = 19,
    COMBAT_ITEM_TARGET_OBJECT = 19,
    COMBAT_ITEM_AMMO = 20,
    COMBAT_ITEM_WEAPON = 21,
    COMBAT_ITEM_END = 22,
    COMBAT_ITEM_COUNT = 5,
    COMBAT_ITEM_USABLE_COUNT = 2,
    TOTAL_COMBAT_LOCATION_COUNT = 22,
    SPECIFIED_VARIABLE_START = -65514,
    ACTIVATE_COMBAT_ITEM = -65514,
    USE_COMBAT_ITEM = -65513,
    EQUIPPED_WEAPON_TYPE = -65512,
    SPECIFIED_VARIABLE_END = -65511,
    SPECIFIED_VARIABLE_COUNT = 3,
    COMBAT_VARIABLE_COUNT = 25,
    ANY_LOCATION = -1,
    ANY_COMBAT_ITEM = -1,
    ANY_WEAPON_TYPE = -1,
    ANY_VALUE = -1,
};

// The low bits come from CombatState::iCombatPermissions; the names are s_combatActionInvestigateTypes'.
enum COMBAT_EXECUTION_FLAGS : int32_t {
    EXECUTION_FLAG_USE_COVER = 0x1,
    EXECUTION_FLAG_CAN_USE_COVER = 0x2,
    EXECUTION_FLAG_AVOID_THREATS = 0x100,
    EXECUTION_FLAG_GROUP_SEARCHING = 0x10000,
    EXECUTION_FLAG_CAN_MOVE = 0x20000,
    // Set when there is no target or the target does not forbid VATS melee.
    EXECUTION_FLAG_TARGET_NO_MELEE = 0x40000,
};

// What a combat plan knows of the world: the value of each COMBAT_WORLD_STATE (a flag set for the bitfield states)
// and the values of the plan's specified variables.
class CombatWorldStateArray {
public:
    CombatWorldStateArray();

    // Zeroes the values.
    void ClearValues();
    // Reads the world state from the controller.
    void Update(CombatController* apCombatController);
    void Copy(CombatWorldStateArray* apWorldStateArray);
    // Sets the state to the value, or for a bitfield state sets the bit of the value.
    void ApplyWorldState(COMBAT_WORLD_STATE aeWorldState, int32_t aiDesiredValue);
    void SetValue(COMBAT_WORLD_STATE aeWorldState, int32_t aiValue);
    // Whether the state holds the value (for a bitfield state: has its bit set).
    bool IsTrue(COMBAT_WORLD_STATE aeWorldState, int32_t aiDesiredValue);
    // aiValue (aiUnsatisfiedValue when aiValue is -1), replaced by the specified variable's value when it names one.
    int32_t GetStateValue(int32_t aiValue, int32_t aiUnsatisfiedValue);
    int32_t GetValue(COMBAT_WORLD_STATE aeWorldState);
    void SetSpecifiedVariable(int32_t aiSpecifiedVariable, int32_t aiValue);
    int32_t GetSpecifiedVariable(int32_t aiSpecifiedVariable);
    // Writes the state's name and values as text to apDebugText.
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, COMBAT_WORLD_STATE aeState);
    CombatWorldStateArray* CombatWorldStateArray_0(CombatWorldStateArray* apWorldStateArray);

    uint32_t iValues[20];
    uint32_t iSpecifiedVariables[3];
};

// A named bitfield world state entry.
class CombatWorldStateData {
public:
    char* pName;
    bool bBitfield;
};

// One world state a combat plan wants satisfied: the state and the value it must hold.
class CombatWorldState {
public:
    // Constructs a state holding aeState and aiValue.
    CombatWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue);

    bool ValueMatches(int32_t aiValue);
    bool Matches(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    // This state's name from kCombatWorldStateData.
    const char* GetStateName();
    // Writes "name: value text" to apDebugText; aiValue -1 stands for iValue.
    void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiValue);

    static bool IsSpecifiedVariable(int32_t aiValue);
    // The state's name from kCombatWorldStateData.
    static const char* GetStateName(COMBAT_WORLD_STATE aeState);
    static const char* GetStateValueText(COMBAT_WORLD_STATE aeState, int32_t aiValue);

    COMBAT_WORLD_STATE eState;
    uint32_t iValue;
};

// A list of world states with the value each must hold: a combat action's preconditions, effects and execution
// conditions.
class CombatWorldStateList : public BSSimpleList<CombatWorldState*> {
public:
    ~CombatWorldStateList();

    // Appends a new entry for aeState holding aiValue.
    void AddWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    // Removes and frees the first entry for aeState.
    void RemoveWorldState(COMBAT_WORLD_STATE aeState);
    // The value of the first entry for aeState in ariActionValue; false when there is none.
    bool GetWorldStateActionValue(COMBAT_WORLD_STATE aeState, int32_t& ariActionValue);
    bool HasMatchingWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    // The next node of the list.
    CombatWorldStateList* GetNext();
};

// Each COMBAT_WORLD_STATE's name and whether it is a bitfield, indexed by the state.
using CombatWorldStateDataTable = CombatWorldStateData[COMBAT_WORLD_STATE_COUNT];
inline CombatWorldStateDataTable kCombatWorldStateData{
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_DEAD")},
    {const_cast<char*>("COMBAT_WORLD_STATE_AT_LOCATION"), true},
    {const_cast<char*>("COMBAT_WORLD_STATE_NOT_AT_LOCATION"), true},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_LOST")},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_DETECTED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_360_LOS")},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_BLOCKED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT")},
    {const_cast<char*>("COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_TARGET_FLUSHED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_WEAPON_DRAWN")},
    {const_cast<char*>("COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_HAS_WEAPON_TYPE"), true},
    {const_cast<char*>("COMBAT_WORLD_STATE_HAS_COMBAT_ITEM"), true},
    {const_cast<char*>("COMBAT_WORLD_STATE_THREATENED")},
    {const_cast<char*>("COMBAT_WORLD_STATE_WANTS_TO_FLEE")},
    {const_cast<char*>("COMBAT_WORLD_STATE_USED_COMBAT_ITEM")},
    {const_cast<char*>("COMBAT_WORLD_STATE_HIDING")},
    {const_cast<char*>("COMBAT_WORLD_STATE_EXECUTION_FLAGS"), true},
    {const_cast<char*>("COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT")}
};

// The state value name tables CombatWorldState::GetStateValueText and the combat actions' GetName index: the move
// location types, the draw weapon types and the execution flag names. The last is indexed by the
// COMBAT_WORLD_STATE_EXECUTION_FLAGS value: 19 entries, blank where no flag has a name.
using CombatActionMoveTypeTable = const char* [26];
inline CombatActionMoveTypeTable s_combatActionMoveTypes{
    "TARGET_LOCATION",      "NEAR_TARGET_LOCATION", "COVER_LOCATION",       "DODGE_LOCATION",
    "FLANK_LOCATION",       "FALLBACK_LOCATION",    "ADVANCE_LOCATION",     "DESTINATION_LOCATION",
    "GUARD_LOCATION",       "CLUSTER_LOCATION",     "HIDE_LOCATION",        "GRENADE_LOCATION",
    "FLEE_LOCATION",        "LOS_LOCATION",         "SAFE_LOCATION",        "INVESTIGATE_LOCATION",
    "COMBAT_AREA_LOCATION", "COMBAT_ITEM_RESTORE",  "COMBAT_ITEM_BUFF",     "COMBAT_ITEM_TARGET_OBJECT",
    "COMBAT_ITEM_AMMO",     "COMBAT_ITEM_WEAPON",   "ACTIVATE_COMBAT_ITEM", "USE_COMBAT_ITEM",
    "EQUIPPED_WEAPON_TYPE"
};
using CombatActionDrawWeaponTypeTable = const char* [8];
inline CombatActionDrawWeaponTypeTable s_combatActionDrawWeaponTypes{
    "COMBAT_WEAPON_RANGED_EXPLOSIVE",
    "COMBAT_WEAPON_RANGED",
    "COMBAT_WEAPON_MELEE",
    "COMBAT_WEAPON_GRENADE",
    "COMBAT_WEAPON_MINE",
    "COMBAT_WEAPON_THROWN",
    "COMBAT_WEAPON_TYPE_NONE",
    "COMBAT_WEAPON_TYPE_INVALID"
};
using CombatActionInvestigateTypeTable = const char* [19];
inline CombatActionInvestigateTypeTable s_combatActionInvestigateTypes{
    "USE_COVER",
    "CAN_USE_COVER",
    "",
    "",
    "",
    "",
    "",
    "",
    "AVOID_THREATS",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "GROUP_SEARCHING",
    "CAN_MOVE",
    "TARGET_NO_MELEE"
};
