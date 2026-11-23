#include "Fallout/Dialogue/DialogueItem.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ResponseListWrapper.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESResponse.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

DialogueItem::DialogueItem()
  : pCurrentResponse(nullptr), pInfo(nullptr), pTopic(nullptr), pQuest(nullptr), pSpeaker(nullptr) {}

// The item gets its info, topic, quest and speaker only when the topic, the info and the quest are all given; the
// response list is read from the info.
DialogueItem::DialogueItem(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apTopicInfo, TESObjectREFR* apSpeaker)
  : pCurrentResponse(nullptr), pInfo(nullptr) {
    if (apTopic && apTopicInfo && apQuest) {
        pInfo = apTopicInfo;
        pTopic = apTopic;
        pQuest = apQuest;
        pSpeaker = apSpeaker;
        ResponseListWrapper kList;
        apTopicInfo->CloneResponseList(&kList);
        for (TESResponse* pResponse = kList.pHead; pResponse; pResponse = pResponse->pNext) {
            lResponses.AddTail(new DialogueResponse(apQuest, apTopic, apTopicInfo, apSpeaker, pResponse));
        }
    }
}

DialogueItem::~DialogueItem() {
    while (!lResponses.IsEmpty()) {
        delete lResponses.m_item;
        lResponses.RemoveHead();
    }
}

bool DialogueItem::FirstResponse() {
    pCurrentResponse = &lResponses;
    return pCurrentResponse->GetItem() != nullptr;
}

DialogueResponse* DialogueItem::GetCurrentResponse() {
    if (pCurrentResponse) {
        return pCurrentResponse->GetItem();
    }
    return nullptr;
}

void DialogueItem::RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript) {
    if (pInfo && !pInfo->IsRunImmediately()) {
        pInfo->AddTopicList();
        pInfo->RunResult(aeScript, pSpeaker);
    }
}

TESTopicInfo* DialogueItem::GetInfo() {
    return pInfo;
}

TESTopic* DialogueItem::GetTopic() {
    return pTopic;
}

void DialogueItem::InitLoadGame() {
    if (pSpeaker) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSpeaker));
        pSpeaker = dynamic_cast<TESObjectREFR*>(pForm);
    }
}

void DialogueItem::InitItem(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pSpeakerRef = nullptr;
    if (pSpeaker) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSpeaker));
        pSpeakerRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pSpeaker = pSpeakerRef;
}

bool DialogueItem::NextResponse() {
    if (pCurrentResponse) {
        pCurrentResponse = pCurrentResponse->m_pkNext;
    }
    return pCurrentResponse != nullptr;
}

void DialogueItem::SetCurrentResponse(DialogueResponse* apDialogueResponse) {
    for (BSSimpleList<DialogueResponse*>* pList = &lResponses; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (apDialogueResponse == pList->m_item) {
            pCurrentResponse = pList;
            return;
        }
    }
}

// Walks the responses' list nodes; the first node is the item itself.
int16_t DialogueItem::GetDialogueResponseIndex(DialogueResponse* apDialogueResponse) {
    int16_t iIndex = 0;
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        if (apDialogueResponse == pNode->m_item) {
            return iIndex;
        }
        ++iIndex;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue response index, the dialogue item was not found in the dialogue "
        "responses list."
    );
    return 0;
}

DialogueResponse* DialogueItem::GetDialogueResponseByIndex(int16_t aiIndex) {
    int16_t iIndex = 0;
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        if (iIndex == aiIndex) {
            return pNode->m_item;
        }
        ++iIndex;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue response by its index, the index was larger than the size of the "
        "dialogue responses list."
    );
    return nullptr;
}

void DialogueItem::SaveGameTES() {
    char cResponseCount = 0;
    uint8_t* pCountSlot = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&cResponseCount, sizeof(cResponseCount));
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        pNode->m_item->SaveGame_0();
        ++cResponseCount;
    }
    *pCountSlot = cResponseCount;
    char cCurrentResponse = -1;
    if (pCurrentResponse) {
        cCurrentResponse = GetDialogueResponseIndex(pCurrentResponse->m_item);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cCurrentResponse, sizeof(cCurrentResponse));
    uint32_t uiInfoID = 0;
    if (pInfo) {
        uiInfoID = pInfo->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&uiInfoID, 4u);
    uint32_t uiTopicID = 0;
    if (pTopic) {
        uiTopicID = pTopic->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&uiTopicID, 4u);
    uint32_t uiQuestID = 0;
    if (pQuest) {
        uiQuestID = pQuest->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&uiQuestID, 4u);
    uint32_t uiSpeakerID = 0;
    if (pSpeaker) {
        uiSpeakerID = pSpeaker->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&uiSpeakerID, 4u);
}

void DialogueItem::LoadGame() {
    uint8_t ucCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&ucCount, sizeof(ucCount));
    for (uint32_t i = 0; i < ucCount; ++i) {
        DialogueResponse* pResponse = new DialogueResponse;
        pResponse->LoadGame();
        lResponses.AddTail(pResponse);
    }
    // The saved index is a signed byte; -1 means no current response.
    int8_t cCurrentResponse;
    pSaveLoadGameOLD->LoadGameDataOLD(&cCurrentResponse, sizeof(cCurrentResponse));
    if (cCurrentResponse == -1) {
        pCurrentResponse = nullptr;
    }
    else {
        DialogueResponse* pResponse = GetDialogueResponseByIndex(cCurrentResponse);
        SetCurrentResponse(pResponse);
    }
    uint32_t uiInfoID;
    pSaveLoadGameOLD->LoadNumericID(&uiInfoID, 4u);
    if (uiInfoID) {
        TESForm* pForm = TESForm::GetFormByNumericID(uiInfoID);
        pInfo = dynamic_cast<TESTopicInfo*>(pForm);
    }
    uint32_t uiTopicID;
    pSaveLoadGameOLD->LoadNumericID(&uiTopicID, 4u);
    if (uiTopicID) {
        TESForm* pForm = TESForm::GetFormByNumericID(uiTopicID);
        pTopic = dynamic_cast<TESTopic*>(pForm);
    }
    uint32_t uiQuestID;
    pSaveLoadGameOLD->LoadNumericID(&uiQuestID, 4u);
    if (uiQuestID) {
        TESForm* pForm = TESForm::GetFormByNumericID(uiQuestID);
        pQuest = dynamic_cast<TESQuest*>(pForm);
    }
    uint32_t uiSpeakerID;
    pSaveLoadGameOLD->LoadNumericID(&uiSpeakerID, 4u);
    // The speaker stays a form ID until InitLoadGame resolves it.
    pSpeaker = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiSpeakerID));
}

void DialogueItem::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t iSaveGameCount = 0;
    uint32_t iSaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    // Walks the nodes with no IsEmpty test and skips empty items.
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode; pNode = pNode->m_pkNext) {
        DialogueResponse* pResponse = pNode->m_item;
        if (pResponse) {
            pResponse->SaveGame(apSaveGameBuffer);
            ++iSaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(iSaveGameCount, iSaveGameBufferPos);
    int16_t sCurrentResponse;
    if (pCurrentResponse) {
        sCurrentResponse = GetDialogueResponseIndex(pCurrentResponse->m_item);
    }
    else {
        sCurrentResponse = -1;
    }
    apSaveGameBuffer->SaveDataEndian(&sCurrentResponse, sizeof(sCurrentResponse), 0);
    apSaveGameBuffer->SaveFormID(pInfo, 0);
    apSaveGameBuffer->SaveFormID(pTopic, 0);
    apSaveGameBuffer->SaveFormID(pQuest, 0);
    apSaveGameBuffer->SaveFormID(pSpeaker, 0);
}

void DialogueItem::LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t iSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iSaveGameCount; ++i) {
        DialogueResponse* pResponse = new DialogueResponse;
        pResponse->LoadGame(apLoadGameBuffer);
        lResponses.AddTail(pResponse);
    }
    int16_t sCurrentResponse = -1;
    apLoadGameBuffer->LoadData(&sCurrentResponse, sizeof(sCurrentResponse));
    if (sCurrentResponse != -1) {
        DialogueResponse* pResponse = GetDialogueResponseByIndex(sCurrentResponse);
        SetCurrentResponse(pResponse);
    }
    TESForm* pInfoForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pInfo = dynamic_cast<TESTopicInfo*>(pInfoForm);
    TESForm* pTopicForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pTopic = dynamic_cast<TESTopic*>(pTopicForm);
    TESForm* pQuestForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
    pQuest = dynamic_cast<TESQuest*>(pQuestForm);
    // The speaker stays a form ID until InitItem resolves it: the field is handed to LoadFormID as an untyped form ID
    // slot.
    apLoadGameBuffer->LoadFormID(pSpeaker);
}

void DialogueItem::PrecacheSounds() {
    // Walks the responses' list nodes; the first node is the item itself.
    BSSimpleList<DialogueResponse*>* pNode = &lResponses;
    while (pNode && !pNode->IsEmpty()) {
        if (pNode->m_item->cText.pString && BSstrlen(pNode->m_item->cText.pString) > 1) {
            char* pcVoice = pNode->m_item->GetVoice()->pString;
            BSAudio::QInstance().PrecacheSound(pcVoice, 0x20100001u, nullptr);
        }

        pNode = pNode->m_pkNext;
    }
}

// The save size: the response count byte, each response, the current response byte and four form IDs.
uint16_t DialogueItem::GetSaveSize() {
    uint16_t usSize = 0;
    usSize += 1;
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        DialogueResponse* pResponse = pNode->m_item;
        usSize += pResponse->GetSaveSize();
    }
    usSize += 1;
    usSize += 4;
    usSize += 4;
    usSize += 4;
    usSize += 4;
    return usSize;
}

void DialogueItem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

DialogueResponse::~DialogueResponse() {}
