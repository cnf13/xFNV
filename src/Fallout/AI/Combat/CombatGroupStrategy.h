#pragma once

#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

class CombatGroup;
class CombatGroupData;

// Base of the combat group's tactics (fallback, advance, flank, ambush): whether it can run, how it starts and its
// per-update step.
class CombatGroupStrategy {
public:
	// Values from the Xbox PDB.
	enum COMBAT_GROUP_STRATEGY : int32_t {
		COMBAT_GROUP_STRATEGY_NONE = -1,
		COMBAT_GROUP_STRATEGY_FALLBACK = 0x0,
		COMBAT_GROUP_STRATEGY_ADVANCE = 0x1,
		COMBAT_GROUP_STRATEGY_FLANK = 0x2,
		COMBAT_GROUP_STRATEGY_AMBUSH = 0x3,
		COMBAT_GROUP_STRATEGY_COUNT = 0x4,
	};

	// Values from the Xbox PDB.
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
	// IDA name; not in the PDB. The name of each strategy type, indexed by it (GetName 0x98FDD0, LookupByName's four
	// entries 0x990250).
	using NameTable = char* [COMBAT_GROUP_STRATEGY_COUNT];

	CombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType);

	virtual bool CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData);
	virtual void Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {}
	virtual bool Update(CombatGroup* apGroup);

	// eType; the exe's shared one-field getter (thiscall, ret 0, 0x726070).
	COMBAT_GROUP_STRATEGY GetType();
	char* GetName();

	// Builds the strategy of each type.
	static void InitGroupStrategies();
	static CombatGroupStrategy* GetCombatGroupStrategy(uint32_t aiIndex);
	// Inferred type: return: CombatGroupStrategy* from 0x990301 (result of
	// CombatGroupStrategy::GetCombatGroupStrategy); a1: char* from 0x990266 (passed to BSstrcpy 2)
	static CombatGroupStrategy* LookupByName(char* a1);

	COMBAT_GROUP_STRATEGY eType;

	static inline StrategyTable pCombatGroupStrategies{};
	static inline NameTable kNames{const_cast<char*>("COMBAT_GROUP_STRATEGY_FALLBACK"),
		const_cast<char*>("COMBAT_GROUP_STRATEGY_ADVANCE"), const_cast<char*>("COMBAT_GROUP_STRATEGY_FLANK"),
		const_cast<char*>("COMBAT_GROUP_STRATEGY_AMBUSH")};
};

// IDA's InitNthCombatGroupStrategy(COMBAT_GROUP_STRATEGY) (cdecl, 0x98FDF0).
// Inferred type: aeType: CombatGroupStrategy::COMBAT_GROUP_STRATEGY: passed to CombatGroupStrategyFallBack::Construct 1
// at 0x98fe4c
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

inline constexpr fnvd::Vtable CombatGroupStrategy_vftable{};
inline constexpr fnvd::Vtable CombatGroupStrategyFallBack_vftable{};
inline constexpr fnvd::Vtable CombatGroupStrategyFlank_vftable{};
inline constexpr fnvd::Vtable CombatGroupStrategyAmbush_vftable{};
inline constexpr fnvd::Vtable CombatGroupStrategyAdvance_vftable{};

// The game settings CombatGroupStrategy.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDisableCombatGroupStrategies;
