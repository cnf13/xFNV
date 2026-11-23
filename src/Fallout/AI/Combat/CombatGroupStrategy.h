#pragma once

#include "BSMain/Setting.h"

class CombatGroup;
class CombatGroupData;

// Base of the combat group's tactics (fallback, advance, flank, ambush): whether it can run, how it starts and its
// per-update step.
class CombatGroupStrategy {
public:
    enum COMBAT_GROUP_STRATEGY : int32_t {
        COMBAT_GROUP_STRATEGY_NONE = -1,
        COMBAT_GROUP_STRATEGY_FALLBACK = 0x0,
        COMBAT_GROUP_STRATEGY_ADVANCE = 0x1,
        COMBAT_GROUP_STRATEGY_FLANK = 0x2,
        COMBAT_GROUP_STRATEGY_AMBUSH = 0x3,
        COMBAT_GROUP_STRATEGY_COUNT = 0x4,
    };

    enum COMBAT_GROUP_STRATEGY_ASSIGNMENT : int32_t {
        STRATEGY_ASSIGNMENT_NONE = 0x0,
        STRATEGY_ASSIGNMENT_FLANKING = 0x1,
        STRATEGY_ASSIGNMENT_FALLING_BACK = 0x2,
        STRATEGY_ASSIGNMENT_AMBUSHING = 0x3,
        STRATEGY_ASSIGNMENT_ADVANCING = 0x4,
        STRATEGY_ASSIGNMENT_COUNT = 0x5,
    };

    // One strategy per COMBAT_GROUP_STRATEGY, built by InitGroupStrategies.
    using StrategyTable = CombatGroupStrategy* [COMBAT_GROUP_STRATEGY_COUNT];
    // The name of each strategy type, indexed by it.
    using NameTable = char* [COMBAT_GROUP_STRATEGY_COUNT];

    CombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

    virtual bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData);
    virtual void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {}
    virtual bool Update(CombatGroup* apGroup);

    // Returns eType.
    COMBAT_GROUP_STRATEGY GetType();
    char* GetName();

    // Builds the strategy of each type.
    static void InitGroupStrategies();
    static CombatGroupStrategy* GetCombatGroupStrategy(uint32_t aiIndex);
    static CombatGroupStrategy* LookupByName(char* apName);

    COMBAT_GROUP_STRATEGY eType;

    static inline StrategyTable pCombatGroupStrategies{};
    static inline NameTable kNames{
        const_cast<char*>("COMBAT_GROUP_STRATEGY_FALLBACK"),
        const_cast<char*>("COMBAT_GROUP_STRATEGY_ADVANCE"),
        const_cast<char*>("COMBAT_GROUP_STRATEGY_FLANK"),
        const_cast<char*>("COMBAT_GROUP_STRATEGY_AMBUSH")
    };
};

CombatGroupStrategy* InitNthCombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);
void DestroyCombatGroupStrategies();

// The fallback tactic: every member falls back.
class CombatGroupStrategyFallBack : public CombatGroupStrategy {
public:
    CombatGroupStrategyFallBack(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

    bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    bool Update(CombatGroup* apGroup) override;
};

// The flank tactic: some members move round the target's side.
class CombatGroupStrategyFlank : public CombatGroupStrategy {
public:
    CombatGroupStrategyFlank(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

    bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    bool Update(CombatGroup* apGroup) override;
};

// The ambush tactic: half the group falls back and the rest lie in wait.
class CombatGroupStrategyAmbush : public CombatGroupStrategy {
public:
    CombatGroupStrategyAmbush(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

    bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    bool Update(CombatGroup* apGroup) override;
};

// The advance tactic: the ranged members advance.
class CombatGroupStrategyAdvance : public CombatGroupStrategy {
public:
    CombatGroupStrategyAdvance(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

    bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) override;
    bool Update(CombatGroup* apGroup) override;
};

extern SettingT<INISettingCollection> bDisableCombatGroupStrategies;
