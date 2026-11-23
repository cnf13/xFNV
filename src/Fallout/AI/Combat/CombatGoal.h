#pragma once

#include "Fallout/AI/Combat/CombatWorldState.h"

class CombatController;

// A goal of the combat planner: the world states that activate it and the states it aims at, plus its type.
class CombatGoal {
public:
    enum COMBAT_GOAL : int32_t {
        COMBAT_GOAL_STAY_IN_COMBAT_AREA = 0x0,
        COMBAT_GOAL_USE_COMBAT_ITEM = 0x1,
        COMBAT_GOAL_FLEE = 0x2,
        COMBAT_GOAL_AVOID_THREATS = 0x3,
        COMBAT_GOAL_ACTIVATE_COMBAT_ITEM = 0x4,
        COMBAT_GOAL_REACH_DESTINATION = 0x5,
        COMBAT_GOAL_GUARD = 0x6,
        COMBAT_GOAL_CLUSTER = 0x7,
        COMBAT_GOAL_ADVANCE = 0x8,
        COMBAT_GOAL_AMBUSH = 0x9,
        COMBAT_GOAL_FALLBACK = 0xa,
        COMBAT_GOAL_FLANK = 0xb,
        COMBAT_GOAL_FIND_TARGET = 0xc,
        COMBAT_GOAL_FLUSH_TARGET = 0xd,
        COMBAT_GOAL_KILL_TARGET = 0xe,
        COMBAT_GOAL_HIDE_FROM_TARGET = 0xf,
        COMBAT_GOAL_COUNT = 0x10,
    };

    CombatGoal(CombatGoal::COMBAT_GOAL aeType);
    // Not virtual: the vtable has no destructor; DeleteGoals calls the deleting destructor directly.
    ~CombatGoal();

    virtual bool IsActive(CombatWorldStateArray* apWorldStateArray);
    virtual void CombatPlannerFailed(CombatController* apCombatController, CombatWorldStateArray* apWorldStateArray) {}

    uint32_t GetGroupStrategyAssignment();
    void AddActivationCondition(COMBAT_WORLD_STATE aeState, int32_t aiValue);
    void AddGoalState(COMBAT_WORLD_STATE aeState, int32_t aiValue);

    // The goal object of type aiGoal.
    static CombatGoal* GetGoal(uint32_t aiGoal);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void InitGoals();
    // Deletes the goals of CombatPlan::pGoals.
    static void DeleteGoals();

    CombatWorldStateList ActivationConditions;
    CombatWorldStateList GoalStates;
    COMBAT_GOAL eType;
    uint32_t iGroupStrategyAssignment;
};

// Creates the goal of type aeGoal. Switches over the 16 goal types and passes the value to each goal's constructor as
// its COMBAT_GOAL.
CombatGoal* InitCombatGoal(CombatGoal::COMBAT_GOAL aeGoal);
