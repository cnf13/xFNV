#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TopicInfoArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"

#include <stdint.h>

class DialogueItem;
class TESObjectREFR;
class TESQuest;
class TESTopic;
class TESTopicInfo;

// Defined in TESTopicInfo.h.
enum DUMMY_NODE_STATE : int32_t;

// One INFC link of a quest's infos: Load stores the info's form ID in nID, a null pInfo and an nDesiredIndex of -1.
struct INFO_LINK_ELEMENT {
    uint32_t nID;
    TESTopicInfo* pInfo;
    int32_t nDesiredIndex;
};

// The kinds of topic.
enum DIALOGUE_TYPE : int32_t {
    DT_TOPIC = 0,
    DT_CONVERSATION = 1,
    DT_COMBAT = 2,
    DT_PERSUASION = 3,
    DT_DETECTION = 4,
    DT_SERVICE = 5,
    DT_MISCELLANEOUS = 6,
    DT_RADIO = 7,
    DIALOGUE_TYPE_COUNT = 8,
};

// The combat dialogue topics, the third argument of CombatDialogueManager::StartDialogue.
enum DIALOGUE_COMBAT : int32_t {
    DC_ATTACK = 0,
    DC_POWER_ATTACK = 1,
    DC_HIT = 2,
    DC_FLEE = 3,
    DC_FIRE_EXPLOSIVE = 4,
    DC_AVOID_THREAT = 5,
    DC_DEATH = 6,
    DC_IDLE = 7,
    DC_GROUP_STRATEGY = 8,
    DC_COMBAT_COUNT = 9,
    DC_STEAL = 9,
    DC_YIELD = 10,
    DC_ACCEPT_YIELD = 11,
    DC_PICKPOCKET = 12,
    DC_ASSAULT = 13,
    DC_MURDER = 14,
    DC_ASSAULT_NC = 15,
    DC_MURDER_NC = 16,
    DC_PICKPOCKET_NC = 17,
    DC_STEAL_NC = 18,
    DC_TRESPASS_NC = 19,
    DC_GUARD_TRESPASS = 20,
    DIALOGUE_COMBAT_COUNT = 21,
};

// One quest's infos of a topic (allocated with ourMalloc, 0x34 bytes).
struct QUEST_INFO {
    QUEST_INFO();

    // The address of infoArray.
    TopicInfoArray* GetInfoArray();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // the quest; holds its form ID until the forms are resolved (bInitialized)
    TESQuest* pQuest;
    TopicInfoArray infoArray;
    BSSimpleArray<INFO_LINK_ELEMENT> infoLinkArray;
    TESQuest* pRemovedQuest;
    bool bInitialized;
};

// The dialogue type and flags of a topic (the DATA chunk).
struct DIALOGUE_DATA {
    enum DIALOGUE_FLAGS : uint8_t {
        FLAG_REMAPPED_RUMORS = 0x1,
        FLAG_RANDOM = 0x2,
        FLAG_DUMMY_ONLY = 0x10,
        FLAG_NON_DUMMY_ONLY = 0x20,
    };

    uint8_t type;
    uint8_t cFlags;
};

// One built-in topic of a dialogue category: the topic once created, its form ID and its editor ID.
struct DEFAULT_TOPIC {
    TESTopic* pTopic;
    uint32_t iID;
    const char* pEdID;
};

// A dialogue topic (DIAL) and its responses.
class TESTopic : public TESForm, public TESFullName {
public:
    // Per dialogue type (8), the built-in topics and their count. The counts are ints: an int index is compared against
    // them signed, an unsigned one unsigned.
    using DefaultTopicArray = DEFAULT_TOPIC* [8];
    using HardcodedTopicCountArray = int32_t[8];

    explicit TESTopic(uint8_t aeType);
    ~TESTopic() override;

    void InitializeData() override;
    bool Load(TESFile* pFile) override;
    void Save() override;
    bool Save(TESFile* apFile) override;
    bool SaveEdit(TESFile* apFile) override;
    bool SavesBefore(TESForm* apForm) override;
    bool SavesBefore(FORM* apGroupFORM) override;
    void InitItem() override;
    bool IsParentForm() override {
        return true;
    }
    bool IsFormTypeChild(uint8_t aeType) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;

    // The first info that applies, outside a conversation.
    TESTopicInfo* GetMatchingInfo(bool& abDispositionFailure, TESObjectREFR* apSpeaker, TESObjectREFR* apTarget);
    // The first info that applies between apSpeaker and apTarget.
    TESTopicInfo* GetMatchingInfo(TESObjectREFR* apSpeaker, TESObjectREFR* apTarget);
    // The first info that applies.
    TESTopicInfo* GetMatchingInfo(
        bool& abDispositionFailure,
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        bool abIsConversation,
        TESTopic* apPreviousTopic,
        BSSimpleList<DialogueItem*>* apConversationList
    );
    // The quest that owns apInfo among the topic's quests, or null.
    TESQuest* GetOwnerQuest(TESTopicInfo* apInfo);
    // Whether the topic has infos of apQuest.
    bool HasQuest(TESQuest* apQuest, bool bRequireInfos);
    // Sorts the topic's quests; false on failure.
    bool SortQuests();
    // The quest list.
    BSSimpleList<QUEST_INFO*>* GetQuestInfoList();
    TopicInfoArray* GetInfoArray(TESQuest* apQuest);
    QUEST_INFO* GetQuestInfoElement(TESTopicInfo* apInfo);
    // Sets the prompt shown for the topic in the dialogue menu.
    void SetDummyPrompt(const char* apPrompt);
    // The entry of apQuest (a form ID while loading), created at the end of the list when missing.
    QUEST_INFO* InsertQuest(TESQuest* apQuest, bool abReSort);
    // The topic's priority, m_fPriority.
    float GetPriority();
    // Sets m_fPriority.
    void SetPriority(float fP);
    bool IsRandom();
    // The dialogue item of the info apSpeaker would say to apTarget, or null.
    DialogueItem* CreateDialogueItem(
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        TESTopic* apPreviousTopic,
        BSSimpleList<DialogueItem*>* apConversationList,
        bool abRunImmediate
    );
    int32_t HasLowIntDialog();
    DIALOGUE_TYPE GetDialogueType() const;
    TESTopicInfo* GetInfoByID(uint32_t aiInfoID, bool abReverse);
    bool IsPersuasion();
    // Which nodes the topic offers by the player's intelligence: DUM_DUMMY_ONLY with DATA flag 0x10, else
    // DUM_NON_DUMMY_ONLY with flag 0x20 and DUM_EITHER without.
    DUMMY_NODE_STATE GetDummyNodeState();
    // Whether the topic's rumors flag (DATA flag 1) is set.
    bool IsRemappedRumors() const;
    bool InsertInfo(TESQuest* apQuest, TESTopicInfo* apInfo, uint32_t auiPrevInfoID);
    void ClearSaidOnceInfosForQuest(TESQuest* apQuest);

    // The hardcoded topic of the category aeType at aiIndex, null when out of range.
    static TESTopic* GetTopic(DIALOGUE_TYPE type, int32_t index);
    // Re-sorts the quests of every topic that has apQuest (all topics when apQuest is null); false once a sort fails.
    static bool SortQuestsOnAllTopics(TESQuest* apQuest);
    // The topic whose info list holds pInfo, or null.
    static TESTopic* GetOwnerTopic(TESTopicInfo* pInfo);
    // Sorts apTopicList, by priority when abSortByPriority.
    static void SortTopics(bool abSortByPriority, BSSimpleList<TESTopic*>* apTopicList);
    // Creates the built-in topics the loaded files did not define.
    static void AddDefaultTopics();
    // Inserts apTopic into apTopicList in sorted order, by priority when abUsePriority.
    static void AddTopicToSortedList(TESTopic* apTopic, BSSimpleList<TESTopic*>* apTopicList, bool abUsePriority);
    // Whether apTopic1 sorts before apTopic2.
    static bool CompareTopics(TESTopic* apTopic1, TESTopic* apTopic2, bool abUsePriority);
    // Info aiInfoIndex of apQuestInfo, or null; the index is compared unsigned.
    static TESTopicInfo* GetInfoByIndex(QUEST_INFO* apQuestInfo, uint32_t aiInfoIndex);
    static void SetShowConversationStats(bool abShow);
    // Reads bShowConversationStats.
    static bool GetShowConversationStats();
    // Whether a form of type aeType belongs under a topic (an info).
    static bool IsFormTypeDialogueChild(uint8_t aeType);
    static void ClearAllSaidOnceInfosForQuest(TESQuest* apQuest);
    // Both callers (InsertInfo, TESTopicInfo::RemoveInfoFromTopic) pass a quest's info array.
    static void SetTopicIndices(NiTLargePrimitiveArray<TESTopicInfo*>* apInfoArray);
    static void RemoveDefaultTopics();
    static TESTopicInfo* GetMatchingInfo(
        const TESTopic* apTopic,
        bool& abDispositionFailure,
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        bool abIsConversation,
        TESTopic* apPreviousTopic,
        BSSimpleList<DialogueItem*>* apConversationList
    );
    static TESTopicInfo* GetMatchingFollowUpInfo(
        TESTopicInfo* apTopicInfo,
        bool& abDispositionFailure,
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        bool abIsConversation,
        BSSimpleList<DialogueItem*>* apConversationList
    );
    static uint32_t CreateConversation(
        TESObjectREFR* apSpeaker,
        TESObjectREFR* apTarget,
        BSSimpleList<DialogueItem*>* apConversationList,
        TESTopic* apStartingTopic
    );

    DIALOGUE_DATA m_Data;
    float m_fPriority;
    BSSimpleList<QUEST_INFO*> m_listQuestInfo;
    BSStringT<char> cDummyPrompt;
    int32_t m_iJournalIndex;
    BSStringT<char> strEditorID;

    // the topic whose responses are being loaded
    static inline TESTopic* pLastTopic;
    static DefaultTopicArray pDefaultTopics;
    static inline HardcodedTopicCountArray uiNumHardcodedTopicsPerCategory{15, 6, 21, 0, 10, 9, 3, 1};
};
