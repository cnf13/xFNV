#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class TESFile;
class TESQuest;
class TESQuestTarget;

// A quest objective's state: bit 0 displayed, values above 1 completed (GetCompleted compares signed).
enum QUEST_OBJECTIVE_STATE : int32_t {
	QOS_DORMANT = 0,
	QOS_DISPLAYED = 1,
	QOS_COMPLETED = 2,
	QOS_COMPLETED_DISPLAYED = 3,
};

// One objective of a quest: its index, display text, the targets that point the player at it and its state (bit 0
// displayed, higher values completed). The only virtual is the destructor. IDA names the members uiIndex,
// strDisplayText and kTargets.
class BGSQuestObjective {
public:
	BGSQuestObjective();
	virtual ~BGSQuestObjective();

	bool GetCompleted();
	// The display text's characters.
	const char* GetDisplayText();
	bool GetDisplayed();
	// KEEP: the forwarder stays (0x44EDB0). An exe caller reads ecx and edx after the call, relying on the exe body
	// leaving them as it does, which clang does not guarantee.
	// The quest that owns the objective (thiscall; the exe shares the one-field getter
	// 0x44EDB0 of +0x10).
	TESQuest* GetOwner();
	// Sets pOwnerQuest; the exe's shared one-field setter (thiscall, ret 4, 0x437730).
	void SetOwner(TESQuest* apQuest);
	// iIndex; the exe's shared one-field getter (thiscall, ret 0, 0x726070).
	uint32_t GetIndex();
	// Deletes every target. ClearData calls it out of line.
	void ClearTargetList();
	void ClearData();
	// Loads the objective's index and display text from apFile's chunks.
	void Load(TESFile* apFile);
	// Resolves the targets for apUser once and records it as the owner.
	void InitItem(TESQuest* apUser);
	// &ListTargets.
	BSSimpleList<TESQuestTarget*>* GetTargetList() { return &ListTargets; }
	void SetDisplayText(const char* apDisplayText);
	// Sets the objective's state (thiscall, ret 4, 0x5EC5D0). PDB: aeNewState is a QUEST_OBJECTIVE_STATE.
	void SetState(QUEST_OBJECTIVE_STATE aeNewState);
	bool GetStatus(int32_t a2);
	void InitializeData();
	void UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill);
	uint32_t GetDisplayTextLength();
	void Save();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t iIndex;
	BSStringT<char> cDisplayText;
	TESQuest* pOwnerQuest;
	BSSimpleList<TESQuestTarget*> ListTargets;
	bool bInitialized;
	QUEST_OBJECTIVE_STATE eState;
};
inline constexpr fnvd::Vtable BGSQuestObjective_vftable{};
