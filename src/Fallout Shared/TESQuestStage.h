#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"

class Date;
class TESObjectREFR;
class TESQuest;
class TESQuestStage;

// A quest stage item's QSDT data: its flags (1: the stage completes the quest, 2: it fails the quest).
struct QUEST_STAGE_ITEM_DATA {
    uint8_t iFlags;
};

// One item of a quest stage: its flags, the conditions and result script, and the log entry it adds.
class TESQuestStageItem {
public:
    TESQuestStageItem();
    ~TESQuestStageItem();

    void SetHasLogEntry(bool a2);
    // The log entry's index: a byte, not a bool, because callers pass a counter.
    void SetLogEntryID(uint8_t a2);
    // Reads the item's own chunks (SCHR, SCDA, SCRO, SCRV, SLSD, QSDT) and a following NAM0.
    bool Load(TESFile* apFile);
    TESCondition* GetConditionList() {
        return &objConditions;
    }
    // The log entry text of apOwner's record for this item's entry, or null when it has none.
    const char* GetLogEntry(TESForm* apOwner);
    // Flag 1 of m_Data.iFlags: the stage completes the quest.
    bool GetEndQuest();
    // Flag 2 of m_Data.iFlags: the stage fails the quest.
    bool GetFailQuest();
    // ucIndex.
    uint8_t GetIndex() {
        return ucIndex;
    }
    // Runs or schedules the item's result script.
    void Resolve(bool abAlwaysPass);
    // Returns m_pLogDate.
    Date* GetLogDate();
    // Stores apDate in m_pLogDate.
    void SetLogDate(Date* apDate);
    // Returns m_pOwner.
    TESQuest* GetOwner();
    void InitItem(TESForm* apOwner, TESQuestStage* apItem);

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    QUEST_STAGE_ITEM_DATA m_Data;
    TESCondition objConditions;
    Script cResultScript;
    uint32_t m_fileOffset;
    uint8_t ucIndex;
    bool m_bHasLogEntry;
    Date* m_pLogDate;
    TESQuest* m_pOwner;
    TESQuest* m_pNextQuest;
};

// A quest stage's INDX data: its index and whether it is done.
struct QUEST_STAGE_DATA {
    QUEST_STAGE_DATA();

    uint8_t ucIndex;
    uint8_t bIsDone;
};

// The older 8-byte INDX data, which also held a stage marker.
struct QUEST_STAGE_DATA_OLD {
    uint8_t ucIndex;
    bool bIsDone;
    uint8_t padding02[2];
    TESObjectREFR* pStageMarker;
};

// One stage of a quest: its index, whether it is done, and its items.
class TESQuestStage {
public:
    TESQuestStage();
    ~TESQuestStage();

    // Reads the stage's INDX chunk from apFile, in the old 8-byte form or the current 2-byte one.
    void Load(TESFile* apFile);
    // Returns m_Data.ucIndex.
    uint8_t GetIndex();
    // Returns &listStageItems.
    BSSimpleList<TESQuestStageItem*>* GetStageItemList();
    // m_Data.bIsDone, the raw byte.
    uint8_t GetDone() {
        return m_Data.bIsDone;
    }
    // The stage's item with index acIndex, or null.
    TESQuestStageItem* GetStageItem(uint8_t acIndex);
    // Marks the stage done and resolves its items; a done stage again only when apQuest allows repeated stages.
    void SetDone(TESQuest* apQuest, bool abAllItems);
    // Stores abDone in m_Data.bIsDone.
    void SetDone(uint8_t abDone);
    // Deletes every item and empties listStageItems.
    void ClearStageItemList();
    void InitItem(TESForm* apOwner);

    // Deleting frees through ourFree.
    static void operator delete(void* apMemory);

    QUEST_STAGE_DATA m_Data;
    BSSimpleList<TESQuestStageItem*> listStageItems;
};

// The item whose log entry was looked up last.
inline TESQuestStageItem* pCurrentItem;

// The log entry text read last.
extern BSStringT<char> cLogEntry;
