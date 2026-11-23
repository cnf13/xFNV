#pragma once

#include "BSCore/BSSimpleList.h"

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class DialogueItem;
class TESObjectREFR;
class TESTopic;

class Conversation {
public:
    Conversation();
    Conversation(TESObjectREFR* apSpeaker, TESObjectREFR* apListener, TESTopic* apTopic);
    ~Conversation();

    // Deletes every queued dialogue item and clears the current one.
    void ClearList();
    // The item of pCurrentItem's list node.
    DialogueItem* GetCurrentItem();
    // Points pCurrentItem at the list node holding apDialogueItem.
    void SetCurrentItem(DialogueItem* apDialogueItem);
    bool GetNext();
    bool FirstItem();
    int16_t GetDialogueItemIndex(DialogueItem* apDialogueItem);
    DialogueItem* GetDialogueItemByIndex(int16_t aiIndex);
    uint16_t GetSaveSizeTES();
    void SaveGameTES();
    void LoadGameTES();
    void InitLoadGameTES();
    void SaveGameBGS(BGSSaveFormBuffer* apBuffer);
    void LoadGameBGS(BGSLoadFormBuffer* apBuffer);
    // Both callers (FalloutRadio::StationState::InitLoadGame and DialoguePackage::InitLoadGame) pass their load buffer.
    void InitItem(BGSLoadGameBuffer* apLoadGameBuffer);
    // Precaches each item's voice files.
    void PrecacheSounds();

    static void operator delete(void* apMemory);

    // The queued dialogue items and the list node of the current one.
    BSSimpleList<DialogueItem*> lDialogueItems;
    BSSimpleList<DialogueItem*>* pCurrentItem;
};
