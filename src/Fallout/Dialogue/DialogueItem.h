#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESTopicInfo.h"

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class DialogueResponse;
class TESObjectREFR;
class TESQuest;
class TESTopic;

// One info a speaker says: its responses, with the one being said, and the info, topic, quest and speaker.
class DialogueItem {
public:
    DialogueItem();
    DialogueItem(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apTopicInfo, TESObjectREFR* apSpeaker);
    ~DialogueItem();

    // Makes the first response current; false when there is none.
    bool FirstResponse();
    // The response being said, or null.
    DialogueResponse* GetCurrentResponse();
    void RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript);
    TESTopicInfo* GetInfo();
    TESTopic* GetTopic();
    void InitLoadGame();
    // Conversation::InitItem passes its load buffer on.
    void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
    bool NextResponse();
    void SetCurrentResponse(DialogueResponse* apDialogueResponse);
    int16_t GetDialogueResponseIndex(DialogueResponse* apDialogueResponse);
    DialogueResponse* GetDialogueResponseByIndex(int16_t aiIndex);
    void SaveGameTES();
    void LoadGame();
    void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
    void LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer);
    void PrecacheSounds();
    // The save size in bytes.
    uint16_t GetSaveSize();

    static void operator delete(void* apMemory);

    // 0x0 the responses of the info
    BSSimpleList<DialogueResponse*> lResponses;
    // 0x8 the node of the response being said
    BSSimpleList<DialogueResponse*>* pCurrentResponse;
    // 0xC the info being said
    TESTopicInfo* pInfo;
    // 0x10 its topic
    TESTopic* pTopic;
    // 0x14 its quest
    TESQuest* pQuest;
    // 0x18 who says it (the constructor's TESObjectREFR; InitLoadGame casts the loaded form to TESObjectREFR)
    TESObjectREFR* pSpeaker;
};
