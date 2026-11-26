#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"

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
// displayed, higher values completed). The only virtual is the destructor.
class BGSQuestObjective {
public:
    BGSQuestObjective();
    virtual ~BGSQuestObjective();

    bool GetCompleted();
    // The display text's characters.
    const char* GetDisplayText();
    bool GetDisplayed();
    // KEEP: the forwarder stays. A caller reads two scratch registers after the call, relying on the body leaving
    // them as they are, which clang does not guarantee.
    // The quest that owns the objective, pOwnerQuest (thiscall; a one-field getter shared through identical-code
    // folding).
    TESQuest* GetOwner();
    // Sets pOwnerQuest.
    void SetOwner(TESQuest* apQuest);
    uint32_t GetIndex();
    // Deletes every target. ClearData calls it.
    void ClearTargetList();
    void ClearData();
    // Loads the objective's index and display text from apFile's chunks.
    void Load(TESFile* apFile);
    // Resolves the targets for apUser once and records it as the owner.
    void InitItem(TESQuest* apUser);
    // &ListTargets.
    BSSimpleList<TESQuestTarget*>* GetTargetList() {
        return &ListTargets;
    }
    void SetDisplayText(const char* apDisplayText);
    void SetState(QUEST_OBJECTIVE_STATE aeNewState);
    bool GetStatus(int32_t aiStatus);
    void InitializeData();
    void UpdateCurrentTargetList(BSSimpleList<TESQuestTarget*>* apListToFill);
    uint32_t GetDisplayTextLength();
    void Save();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint32_t iIndex;
    BSStringT<char> cDisplayText;
    TESQuest* pOwnerQuest;
    BSSimpleList<TESQuestTarget*> ListTargets;
    bool bInitialized;
    QUEST_OBJECTIVE_STATE eState;
};
