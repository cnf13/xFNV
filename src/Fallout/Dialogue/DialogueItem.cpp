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
    DialogueResponse* pResponse;
    pResponse = pCurrentResponse->GetItem();
    return pResponse != nullptr;
}

DialogueResponse* DialogueItem::GetCurrentResponse() {
    if (pCurrentResponse) {
        DialogueResponse* pResponse;
        pResponse = pCurrentResponse->GetItem();
        return pResponse;
    }
    return nullptr;
}

void DialogueItem::RunResult(TOPIC_INFO_RESULT_SCRIPT aeScript) {
    if ((pInfo) && (!pInfo->IsRunImmediately())) {
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
    TESForm* formByNumericID;
    TESObjectREFR* pSpeaker;
    pSpeaker = this->pSpeaker;
    if (pSpeaker) {
        formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSpeaker));
        this->pSpeaker = dynamic_cast<TESObjectREFR*>(formByNumericID);
    }
}

void DialogueItem::InitItem(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESForm* formByNumericID;
    TESObjectREFR* v3;
    if (pSpeaker) {
        formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSpeaker));
        v3 = dynamic_cast<TESObjectREFR*>(formByNumericID);
    }
    else {
        v3 = nullptr;
    }
    pSpeaker = v3;
}

bool DialogueItem::NextResponse() {
    if (pCurrentResponse) {
        pCurrentResponse = pCurrentResponse->m_pkNext;
    }
    return (pCurrentResponse != nullptr);
}

void DialogueItem::SetCurrentResponse(DialogueResponse* apDialogueResponse) {
    for (BSSimpleList<DialogueResponse*>* plist = &lResponses; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        if (apDialogueResponse == plist->m_item) {
            pCurrentResponse = plist;
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
    int16_t iindex = 0;
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        if (iindex == aiIndex) {
            return pNode->m_item;
        }
        ++iindex;
    }
    Warning(
        "DIALOGUE: When trying to get a dialogue response by its index, the index was larger than the size of the "
        "dialogue responses list."
    );
    return nullptr;
}

void DialogueItem::SaveGameTES() {
    uint32_t v4;
    char DialogueResponseIndex;
    uint32_t a2;
    uint32_t apFormID;
    char apData;
    uint32_t CastingType;
    apData = 0;
    uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&apData, 1u);
    for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        pNode->m_item->SaveGame_0();
        ++apData;
    }
    *BufferPointer = apData;
    DialogueResponseIndex = -1;
    if (pCurrentResponse) {
        DialogueResponseIndex = GetDialogueResponseIndex(pCurrentResponse->m_item);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&DialogueResponseIndex, 1u);
    a2 = 0;
    if (pInfo) {
        a2 = pInfo->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&a2, 4u);
    apFormID = 0;
    if (pTopic) {
        apFormID = pTopic->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&apFormID, 4u);
    CastingType = 0;
    if (pQuest) {
        CastingType = pQuest->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&CastingType, 4u);
    v4 = 0;
    if (pSpeaker) {
        v4 = pSpeaker->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&v4, 4u);
}

void DialogueItem::LoadGame() {
    uint8_t ucCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&ucCount, 1u);
    for (uint32_t i = 0; i < ucCount; ++i) {
        DialogueResponse* pResponse;
        DialogueResponse* pMemory = static_cast<DialogueResponse*>(ourMalloc(sizeof(DialogueResponse)));
        if (pMemory) {
            pResponse = ::new (pMemory) DialogueResponse;
        }
        else {
            pResponse = nullptr;
        }
        pResponse->LoadGame();
        lResponses.AddTail(pResponse);
    }
    // The saved index is a signed byte; -1 means no current response.
    int8_t cCurrentResponse;
    pSaveLoadGameOLD->LoadGameDataOLD(&cCurrentResponse, 1u);
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
    apSaveGameBuffer->SaveDataEndian(&sCurrentResponse, 2u, 0);
    apSaveGameBuffer->SaveFormID(pInfo, 0);
    apSaveGameBuffer->SaveFormID(pTopic, 0);
    apSaveGameBuffer->SaveFormID(pQuest, 0);
    apSaveGameBuffer->SaveFormID(pSpeaker, 0);
}

void DialogueItem::LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t iSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iSaveGameCount; ++i) {
        DialogueResponse* pResponse;
        DialogueResponse* pMemory = static_cast<DialogueResponse*>(ourMalloc(sizeof(DialogueResponse)));
        if (pMemory) {
            pResponse = ::new (pMemory) DialogueResponse;
        }
        else {
            pResponse = nullptr;
        }
        pResponse->LoadGame(apLoadGameBuffer);
        lResponses.AddTail(pResponse);
    }
    int16_t sCurrentResponse = -1;
    apLoadGameBuffer->LoadData(&sCurrentResponse, 2u);
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
        if (((&pNode->m_item->cText)->pString) && (BSstrlen((&pNode->m_item->cText)->pString) > 1)) {
            char* v2 = pNode->m_item->GetVoice()->pString;
            BSAudio* v1 = &BSAudio::QInstance();
            v1->PrecacheSound(v2, 0x20100001u, nullptr);
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
