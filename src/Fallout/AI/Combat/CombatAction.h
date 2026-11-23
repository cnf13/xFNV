#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout/AI/Combat/CombatWorldState.h"

class CombatController;

// Deletes the 27 actions of CombatAction::pCombatActions, then empties and deletes the 20 lists of
// CombatAction::pPerStateLists. CombatManager::Destroy calls it.
void FreeCombatActions();
char SetCombatActionCost(char* apName, float afCost);

class CombatAction {
public:
    // The actions queued for each combat state.
    using PerStateListTable = BSSimpleList<CombatAction*>* [20];

    // A kind of action: its name, cost and default flags.
    struct Descriptor {
        const char* pName;
        float fCost;
        int32_t uiDefaultFlags;
    };

    // The kind of an action (eType).
    enum COMBAT_ACTION : int32_t {
        COMBAT_ACTION_NONE = -1,
        COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE = 0,
        COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE_FROM_COVER = 1,
        COMBAT_ACTION_ATTACK_RANGED = 2,
        COMBAT_ACTION_ATTACK_RANGED_FROM_COVER = 3,
        COMBAT_ACTION_ATTACK_GRENADE = 4,
        COMBAT_ACTION_ATTACK_GRENADE_FLUSH_TARGET = 5,
        COMBAT_ACTION_ATTACK_GRENADE_FROM_COVER = 6,
        COMBAT_ACTION_ATTACK_MELEE = 7,
        COMBAT_ACTION_ATTACK_HAND_TO_HAND = 8,
        COMBAT_ACTION_MOVE = 9,
        COMBAT_ACTION_MOVE_AND_SWITCH_TO_MELEE = 10,
        COMBAT_ACTION_MOVE_AND_ATTACK_RANGED = 11,
        COMBAT_ACTION_MOVE_AND_ATTACK_RANGED_EXPLOSIVE = 12,
        COMBAT_ACTION_MOVE_AND_ATTACK_GRENADE = 13,
        COMBAT_ACTION_DRAW_WEAPON = 14,
        COMBAT_ACTION_SWITCH_WEAPON = 15,
        COMBAT_ACTION_AVOID_THREAT = 16,
        COMBAT_ACTION_SEARCH = 17,
        COMBAT_ACTION_INVESTIGATE = 18,
        COMBAT_ACTION_DODGE = 19,
        COMBAT_ACTION_IGNORE_BLOCKED_TARGET = 20,
        COMBAT_ACTION_FLEE = 21,
        COMBAT_ACTION_ACTIVATE_COMBAT_ITEM = 22,
        COMBAT_ACTION_USE_COMBAT_ITEM = 23,
        COMBAT_ACTION_ACQUIRE_LINE_OF_SIGHT = 24,
        COMBAT_ACTION_HIDE = 25,
        COMBAT_ACTION_APPROACH_TARGET = 26,
        COMBAT_ACTION_COUNT = 27,
    };

    // Bits of iFlags.
    enum COMBAT_ACTION_FLAG : uint32_t {
        INFINITE_ACTION = 1,
        BACKGROUND_ACTION = 2,
    };

    // One entry per COMBAT_ACTION.
    using kDescriptors_t = CombatAction::Descriptor[27];
    // One action per COMBAT_ACTION: InitActions fills and FreeCombatActions deletes 27
    // entries, and the per-TU statics of CombatController.cpp start at +0x70.
    using pCombatActions_t = CombatAction* [COMBAT_ACTION_COUNT];

    CombatAction(COMBAT_ACTION aeType);
    ~CombatAction();

    // PC CanExecute source takes the controller first, despite the stale PC vtable type's world-state argument.
    virtual bool
    CanExecute(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray, int32_t aiActionValue);
    virtual void Execute(CombatController* apCombatController, int32_t aiActionValue) {}
    virtual bool CheckIdentical(CombatController* apCombatController, int32_t aiActionValue);
    virtual float GetCost(CombatController* apCombatController, int32_t aiActionValue);
    virtual void GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiActionValue);

    const char* GetCombatActionInfoName();
    bool GetPreconditionWorldStateActionValue(COMBAT_WORLD_STATE aeState, int32_t& ariActionValue);
    void AddPrecondition(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    void RemovePrecondition(COMBAT_WORLD_STATE aeState);
    void AddExecutionCondition(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    void RemoveExecutionCondition(COMBAT_WORLD_STATE aeState);
    void AddEffect(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    bool GetFlag(COMBAT_ACTION_FLAG aeFlag);
    void AddPossibleEffect(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    void RemoveResult(COMBAT_WORLD_STATE aeState);
    bool HasTargetState(CombatWorldState* apEntry);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void InitActions();
    static void AddWorldStateAction(COMBAT_WORLD_STATE aeCombatState, CombatAction* apAction);
    // Returns pPerStateLists[aeState], a BSSimpleList<CombatAction*>*;
    // its callers walk it as such.
    static BSSimpleList<CombatAction*>* GetActionsForTargetState(COMBAT_WORLD_STATE aeState);
    static CombatAction* CreateCombatAction(CombatAction::COMBAT_ACTION aeCombatAction);

    CombatWorldStateList ExecutionConditions;
    CombatWorldStateList Preconditions;
    CombatWorldStateList Effects;
    CombatWorldStateList PossibleEffects;
    COMBAT_ACTION eType;
    uint32_t iFlags;

    static inline PerStateListTable pPerStateLists{};
    static inline kDescriptors_t kDescriptors{
        {"COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE", 1.5f, 1},
        {"COMBAT_ACTION_ATTACK_RANGED_EXPLOSIVE_FROM_COVER", 1.0f, 1},
        {"COMBAT_ACTION_ATTACK_RANGED", 1.5f, 1},
        {"COMBAT_ACTION_ATTACK_RANGED_FROM_COVER", 1.0f, 1},
        {"COMBAT_ACTION_ATTACK_GRENADE", 1.5f, 1},
        {"COMBAT_ACTION_ATTACK_GRENADE_FLUSH_TARGET", 1.25f, 1},
        {"COMBAT_ACTION_ATTACK_GRENADE_FROM_COVER", 1.0f, 1},
        {"COMBAT_ACTION_ATTACK_MELEE", 2.0f, 1},
        {"COMBAT_ACTION_ATTACK_HAND_TO_HAND", 2.0f, 1},
        {"COMBAT_ACTION_MOVE"},
        {"COMBAT_ACTION_MOVE_AND_SWITCH_TO_MELEE"},
        {"COMBAT_ACTION_MOVE_AND_ATTACK_RANGED"},
        {"COMBAT_ACTION_MOVE_AND_ATTACK_RANGED_EXPLOSIVE"},
        {"COMBAT_ACTION_MOVE_AND_ATTACK_GRENADE"},
        {"COMBAT_ACTION_DRAW_WEAPON", 1.0f, 2},
        {"COMBAT_ACTION_SWITCH_WEAPON", 2.0f, 2},
        {"COMBAT_ACTION_AVOID_THREAT"},
        {"COMBAT_ACTION_SEARCH", 3.0f, 1},
        {"COMBAT_ACTION_INVESTIGATE", 1.0f, 1},
        {"COMBAT_ACTION_DODGE", 0.1f},
        {"COMBAT_ACTION_IGNORE_BLOCKED_TARGET", 1.5f, 2},
        {"COMBAT_ACTION_FLEE", 0.0f, 1},
        {"COMBAT_ACTION_ACTIVATE_COMBAT_ITEM", 1.0f},
        {"COMBAT_ACTION_USE_COMBAT_ITEM", 1.0f},
        {"COMBAT_ACTION_ACQUIRE_LINE_OF_SIGHT", 2.0f},
        {"COMBAT_ACTION_HIDE", 1.0f},
        {"COMBAT_ACTION_APPROACH_TARGET", 1.0f}
    };
    static inline pCombatActions_t pCombatActions{};
};
