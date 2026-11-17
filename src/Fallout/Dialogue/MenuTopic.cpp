#include "Fallout/Dialogue/MenuTopic.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ResponseListWrapper.h"
#include "Fallout Shared/TESConversationData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESResponse.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

MenuTopic::MenuTopic()
  : cTopicText(), lResponses(), pQuest(nullptr), pInfo(nullptr), pCurrentResponse(nullptr), pTopic(nullptr) {
    cTopicText.Set("", 0);
    bInfoRefusal = false;
    bPersuasion = false;
}

MenuTopic::MenuTopic(
    TESQuest* apQuest,
    TESTopic* apTopic,
    TESTopicInfo* apInfo,
    TESObjectREFR* apSpeaker,
    bool abInfoRefusal
)
  : cTopicText(), lResponses(), pQuest(nullptr), pInfo(nullptr), pOrigTopic(apTopic), pCurrentResponse(nullptr),
    pTopic(nullptr) {
    if (apTopic->IsRemappedRumors()) {
        bGeneralTopic = true;
        bTopicIsChoice = false;
    }
    else {
        bGeneralTopic = false;
    }
    bInfoRefusal = false;
    bPersuasion = false;
    TESQuest* pResponseQuest = apQuest;
    TESTopic* pResponseTopic = apTopic;
    if (apTopic && apInfo) {
        bTopicIsNew = !apInfo->bSaidOnce;
        pInfo = apInfo;
        if (!abInfoRefusal || apInfo->IsInfoRefusal()) {
            bInfoRefusal = apInfo->IsInfoRefusal();
        }
        else {
            TESTopic* pRefusalTopic = TESTopic::GetTopic(DT_MISCELLANEOUS, 12);
            TESTopicInfo* pInfoRefusal = pRefusalTopic ? pRefusalTopic->GetMatchingInfo(apSpeaker, pPlayer) : nullptr;
            if (pInfoRefusal) {
                pInfo = pInfoRefusal;
                pResponseTopic = pRefusalTopic;
                pResponseQuest = pRefusalTopic->GetOwnerQuest(pInfoRefusal);
            }
            bInfoRefusal = true;
        }
        int32_t iIntelligence = pPlayer->GetActorValue(ActorValue::INTELLIGENCE);
        if (iIntelligence <= iDialogueDummySpeakThisIntOrBelow.Int() && apTopic->HasLowIntDialog()) {
            cTopicText.Set(apTopic->cDummyPrompt);
        }
        else if (apInfo->HasPrompt()) {
            cTopicText.Set(apInfo->cPrompt);
        }
        else {
            cTopicText.Set(apTopic->GetFullName(), 0);
        }
        BSStringT<char> kStripped;
        cTopicText.Set(*cTopicText.RemoveCurlyContent(&kStripped));
        if (!bGeneralTopic) {
            bTopicIsChoice = pInfo->GetConversationData() && !pInfo->GetConversationData()->m_listLinkTo.IsEmpty();
        }
        if (apTopic->IsPersuasion()) {
            bPersuasion = true;
        }
        TESQuest* pOwnerQuest = pInfo->pOwnerQuest;
        TESTopic* pOwnerTopic = TESTopic::GetOwnerTopic(pInfo);
        if (pResponseQuest == pOwnerQuest && pResponseTopic == pOwnerTopic) {
            FillResponseList(pResponseQuest, pResponseTopic, pInfo, apSpeaker);
        }
        else {
            FillResponseList(pOwnerQuest, pOwnerTopic, pInfo, apSpeaker);
        }
    }
    pTopic = pResponseTopic;
    pQuest = pResponseQuest;
}

MenuTopic::~MenuTopic() {
    if (!bGeneralTopic) {
        // The items are owned; the list's own destructor frees its nodes.
        for (BSSimpleList<DialogueResponse*>* pNode = &lResponses; pNode; pNode = pNode->GetNext()) {
            delete pNode->m_item;
        }
    }
}

bool MenuTopic::GetResponseListEmpty() {
    return ((&lResponses)->ItemsInList() == 0);
}

void MenuTopic::FillResponseList(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apInfo, TESObjectREFR* apSpeaker) {
    // ResponseListWrapper has no C++ destructor.
    ResponseListWrapper lResponseList;
    pInfo->CloneResponseList(&lResponseList);
    TESResponse* presponse = lResponseList.pHead;
    int32_t i = 0;
    while (presponse) {
        if ((presponse->GetResponseTextLength()) && (!bGeneralTopic || !i)) {
            DialogueResponse* pMenuResponse;
            DialogueResponse* apMemory = static_cast<DialogueResponse*>(ourMalloc(sizeof(DialogueResponse)));
            if (apMemory) {
                pMenuResponse = ::new (apMemory) DialogueResponse(apQuest, apTopic, apInfo, apSpeaker, presponse);
            }
            else {
                pMenuResponse = nullptr;
            }
            DialogueResponse* pMenuResponse_2 = pMenuResponse;
            lResponses.AddTail(pMenuResponse_2);
            ++i;
        }

        presponse = presponse->pNext;
    }
}

// The null test of the just stored &lResponses is dead.
bool MenuTopic::FirstResponse() {
    pCurrentResponse = &lResponses;
    bool bHasResponse;
    bHasResponse = pCurrentResponse && pCurrentResponse->GetItem();
    return bHasResponse;
}

bool MenuTopic::NextResponse() {
    if (pCurrentResponse) {
        pCurrentResponse = pCurrentResponse->GetNext();
    }
    bool bHasResponse;
    bHasResponse = pCurrentResponse && pCurrentResponse->GetItem();
    return bHasResponse;
}

DialogueResponse* MenuTopic::GetCurrentResponse() {
    if (pCurrentResponse) {
        return pCurrentResponse->GetItem();
    }
    return nullptr;
}

void MenuTopic::LoadGame(TESObjectREFR* apSpeaker) {
    uint16_t usSize = 0;
    char* pStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiMarker, 4);
        if (uiMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint8_t ucVersion = pHeader->cVersion;
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 773, pHeader->iFormID,
                    pForm->GetFormEditorID(), ucVersion, uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 773,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    }
    char acText[0x104];
    ourMemset(acText, 0, 0x104);
    uint8_t ucLength;
    pSaveLoadGameOLD->LoadGameDataOLD(&ucLength, 1);
    if (ucLength) {
        pSaveLoadGameOLD->LoadGameDataOLD(acText, ucLength);
        cTopicText.Set(acText, 0);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&bTopicIsChoice, 1);
    pSaveLoadGameOLD->LoadGameDataOLD(&bGeneralTopic, 1);
    pSaveLoadGameOLD->LoadGameDataOLD(&bTopicIsNew, 1);
    uint32_t uiQuestID;
    pSaveLoadGameOLD->LoadNumericID(&uiQuestID, 4);
    pQuest = dynamic_cast<TESQuest*>(TESForm::GetFormByNumericID(uiQuestID));
    uint32_t uiTopicID;
    pSaveLoadGameOLD->LoadNumericID(&uiTopicID, 4);
    pTopic = dynamic_cast<TESTopic*>(TESForm::GetFormByNumericID(uiTopicID));
    uint32_t uiInfoID;
    pSaveLoadGameOLD->LoadNumericID(&uiInfoID, 4);
    pInfo = dynamic_cast<TESTopicInfo*>(TESForm::GetFormByNumericID(uiInfoID));
    if (!pSaveLoadGameOLD->UseSaveGameBlocks()) {
        return;
    }
    char* pNow = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
    if (pHeader) {
        TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
        if (pNow > pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>(pNow - (pStart + usSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 809, pHeader->iFormID,
                pForm->GetFormEditorID(), ucVersion, uiFlags
            );
        }
        else if (pNow < pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>((pStart + usSize) - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 809, pHeader->iFormID,
                pForm->GetFormEditorID(), ucVersion, uiFlags
            );
        }
    }
    else if (pNow > pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>(pNow - (pStart + usSize)),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 809,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
    else if (pNow < pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>((pStart + usSize) - pNow),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 809,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
}

uint16_t MenuTopic::GetSaveSize() {
    int16_t v5 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v5 = 6;
    }
    uint16_t isize = BSstrlen(GetTopicText()) + (v5 + 1) + 15;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 729,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 729,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
    }
    return isize;
}

void MenuTopic::SaveGame() {
    uint32_t iblockHeader;
    int16_t v11[3];
    uint8_t clength;
    uint32_t itopicID;
    uint32_t iinfoID;
    uint32_t iquestID;
    v11[0] = 0;
    uint16_t* pblockSize = nullptr;
    // The same position as pblockSize, as the save buffer's bytes, so it can be added and subtracted as an address.
    uint8_t* pblockStart = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pblockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(v11, 2u);
    }
    const char* ptext = GetTopicText();
    clength = BSstrlen(ptext);
    pSaveLoadGameOLD->SaveGameDataOLD(&clength, 1u);
    if (clength) {
        pSaveLoadGameOLD->SaveGameDataOLD(ptext, clength);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&bTopicIsChoice, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&bGeneralTopic, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&bTopicIsNew, 1u);
    iquestID = 0;
    if (pQuest) {
        iquestID = pQuest->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iquestID, 4u);
    itopicID = 0;
    if (pTopic) {
        itopicID = pTopic->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&itopicID, 4u);
    iinfoID = 0;
    if (pInfo) {
        iinfoID = pInfo->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&iinfoID, 4u);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                BufferPointer - itempStartPosition, pheader->iFormID, v1, iFlags, 767,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition, 767,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size is the bytes from its size word to the buffer's position, compared unsigned.
        uint8_t* pBuffer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        if (pBuffer > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp", 767
            );
        }
        *pblockSize = static_cast<uint16_t>(pBuffer - pblockStart);
    }
}

TESTopicInfo* MenuTopic::LoadGame_0083FBE0(BGSLoadGameBuffer* a1) {
    char apString[508];
    a1->LoadString(apString);
    cTopicText.operator=(apString);
    a1->LoadData(&bTopicIsChoice, 1u);
    a1->LoadData(&bGeneralTopic, 1u);
    a1->LoadData(&bTopicIsNew, 1u);
    a1->LoadData(&bInfoRefusal, 1u);
    a1->LoadData(&bPersuasion, 1u);
    uint32_t FormID_0 = a1->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pQuest = dynamic_cast<TESQuest*>(FormByNumericID);
    uint32_t v4 = a1->LoadFormID();
    TESForm* v5 = TESForm::GetFormByNumericID(v4);
    pTopic = dynamic_cast<TESTopic*>(v5);
    uint32_t v6 = a1->LoadFormID();
    TESForm* v7 = TESForm::GetFormByNumericID(v6);
    pOrigTopic = dynamic_cast<TESTopic*>(v7);
    uint32_t v8 = a1->LoadFormID();
    TESForm* v9 = TESForm::GetFormByNumericID(v8);
    TESTopicInfo* result = dynamic_cast<TESTopicInfo*>(v9);
    pInfo = result;
    return result;
}

void MenuTopic::SaveGame_0(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveString(GetTopicText(), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTopicIsChoice), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bGeneralTopic), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTopicIsNew), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bInfoRefusal), 1u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPersuasion), 1u, 0);
    apSaveGameBuffer->SaveFormID(pQuest, 0u);
    apSaveGameBuffer->SaveFormID(pTopic, 0u);
    apSaveGameBuffer->SaveFormID(pOrigTopic, 0u);
    apSaveGameBuffer->SaveFormID(pInfo, 0u);
}

void MenuTopic::operator delete(void* apMemory) {
    ourFree(apMemory);
}

MenuTopicManager::MenuTopicManager() : pCurrentTopic(nullptr), listTopicLists(), pSpeaker(nullptr) {}

MenuTopicManager::~MenuTopicManager() {
    ClearData();
}

bool MenuTopicManager::FirstTopic(bool abSkipGreeting) {
    pCurrentTopic = GetCurrentTopicList();
    if (abSkipGreeting && pCurrentTopic) {
        pCurrentTopic = pCurrentTopic->GetNext();
    }
    if (!pCurrentTopic) {
        return false;
    }
    MenuTopic* pTopic;
    pTopic = pCurrentTopic->GetItem();
    return pTopic != nullptr;
}

BSSimpleList<MenuTopic*>* MenuTopicManager::GetCurrentTopicList() {
    return (&listTopicLists)->m_item;
}

MenuTopic* MenuTopicManager::GetCurrentTopic() {
    if (pCurrentTopic) {
        MenuTopic* pTopic;
        pTopic = pCurrentTopic->GetItem();
        return pTopic;
    }
    return nullptr;
}

// GetInfo is called twice, as the game does.
void MenuTopicManager::RunBeginScript() {
    MenuTopic* pTopic = GetCurrentTopic();
    TESTopicInfo* pInfo = nullptr;
    if (pTopic && !pTopic->GetInfo()->IsRunForRumors()) {
        pInfo = pTopic->GetInfo();
    }
    if (pInfo) {
        pInfo->RunResult(TIRS_BEGIN, pSpeaker);
    }
}

void MenuTopicManager::PushTopicList(BSSimpleList<MenuTopic*>* apList) {
    if (apList) {
        listTopicLists.AddHead(apList);
    }
}

void MenuTopicManager::ClearData() {
    pSpeaker = nullptr;
    while (!listTopicLists.IsEmpty()) {
        BSSimpleList<MenuTopic*>* plist = listTopicLists.m_item;
        listTopicLists.RemoveHead();
        ClearTopicList(plist, listTopicLists.IsEmpty());
        plist->RemoveAllPointerList();
        if (plist) {
            delete plist;
        }
    }
}

bool MenuTopicManager::GoToTopic(int32_t aiIndex) {
    int32_t v6 = 0;
    BSSimpleList<MenuTopic*>* currentTopicList = GetCurrentTopicList();
    if (currentTopicList) {
        currentTopicList = currentTopicList->m_pkNext;
    }
    while (currentTopicList && (v6 < aiIndex)) {
        currentTopicList = currentTopicList->m_pkNext;
        ++v6;
    }
    int8_t v3 = 0;
    if (currentTopicList) {
        pCurrentTopic = currentTopicList;
        if (pCurrentTopic->m_item) {
            return true;
        }
    }
    return (bool)v3;
}

MenuTopic* MenuTopicManager::GetGreeting() {
    BSSimpleList<MenuTopic*>* currentTopicList = GetCurrentTopicList();
    return currentTopicList->m_item;
}

bool MenuTopicManager::NextTopic() {
    if (pCurrentTopic) {
        pCurrentTopic = pCurrentTopic->m_pkNext;
    }
    return (bool)((uint8_t)(pCurrentTopic && pCurrentTopic->m_item));
}

void MenuTopicManager::ClearTopicList(BSSimpleList<MenuTopic*>* apList, bool abClearGreeting) {
    pCurrentTopic = nullptr;
    BSSimpleList<MenuTopic*>* pList = apList;
    if ((!abClearGreeting) && (apList)) {
        pList = apList->m_pkNext;
    }

    while (pList && !pList->IsEmpty()) {
        if (!pList->m_item->bGeneralTopic) {
            MenuTopic* apThis = pList->m_item;
            if (apThis) {
                delete apThis;
            }
        }
        pList->RemoveHead();
    }
}

bool MenuTopicManager::Initialize(TESObjectREFR* apSpeaker, TESTopic* apForceGreeting) {
    BSSimpleList<MenuTopic*>* v6;
    bool bdummyBool;
    ClearData();
    void* v9 = ourMalloc(sizeof(BSSimpleList<MenuTopic*>));
    if (v9) {
        v6 = ::new (v9) BSSimpleList<MenuTopic*>;
    }
    else {
        v6 = nullptr;
    }
    BSSimpleList<MenuTopic*>* a2 = v6;
    listTopicLists.AddHead(a2);
    pSpeaker = apSpeaker;
    bSpeechChallengeLoss = false;
    iLastDialogXP = 0;
    TESTopic* pGreetTopic = apForceGreeting;
    if (!apForceGreeting) {
        pGreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
    }
    MenuTopic* pGreeting = nullptr;
    if (pGreetTopic) {
        TESTopicInfo* pInfo = pGreetTopic->GetMatchingInfo(bdummyBool, pSpeaker, pPlayer);
        if (pInfo) {
            MenuTopic* pGreeting_2;
            TESQuest* pQuest = pGreetTopic->GetOwnerQuest(pInfo);
            MenuTopic* apMemory = static_cast<MenuTopic*>(ourMalloc(sizeof(MenuTopic)));
            if (apMemory) {
                pGreeting_2 = ::new (apMemory) MenuTopic(pQuest, pGreetTopic, pInfo, pSpeaker, false);
            }
            else {
                pGreeting_2 = nullptr;
            }
            pGreeting = pGreeting_2;
            pGreeting_2->pTopic->SetPriority(2.1474836e+09f);
            pGreeting->FirstResponse();
            GetCurrentTopicList()->AddTail(pGreeting);
        }
    }
    if (pGreeting) {
        pGreeting->pInfo->AddTopicList();
        pGreeting->pInfo->RunResult(TIRS_BEGIN, pSpeaker);
    }
    else {
        Warning(
            WARN_DIALOGUE, "No GREETING topic was found for %s. Selecting stock GREETING.", pSpeaker->GetFullName()
        );
    }
    FirstTopic(false);
    return pGreeting && (pGreeting->pInfo && pGreeting->pInfo->IsGoodbye());
}

bool MenuTopicManager::DoSpeechChallengeCheck(MenuTopic* pSpeechTopic) {
    bool bdummy;
    if ((pSpeechTopic) && (pSpeechTopic->pInfo->IsSpeechChallenge())) {
        float fDisposition;
        if (pSpeaker->IsActor()) {
            fDisposition = static_cast<float>(static_cast<Actor*>(pSpeaker)->GetDisposition(pPlayer, nullptr));
        }
        else {
            fDisposition = fSpeechChallengeDispositionBase.Float();
        }
        uint32_t SpeechChallengeDifficultyMult = pSpeechTopic->pInfo->GetSpeechChallengeDifficultyMult();
        float fSpeech = pPlayer->GetClampedActorFloatValue(ActorValue::SPEECH);
        int32_t iChallengeChance =
            GamePlayFormulas::GetSpeechChallengeChance(fSpeech, fDisposition, SpeechChallengeDifficultyMult);
        int32_t idieRoll = Random() % 100;
        if (idieRoll < iChallengeChance) {
            int32_t State = pSpeechTopic->pInfo->eDifficulty;
            iLastDialogXP =
                ExperiencePoints::GetExperiencePoints(ExperiencePoints::ACTIVITY_SPEECH_CHALLENGE, State - 1);
        }
        else {
            bSpeechChallengeLoss = true;
            MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
            TESTopic* pspeechFailTopic = TESTopic::GetTopic(DT_TOPIC, 11);
            TESTopic* afDisposition = pSpeechTopic->pTopic;
            TESTopicInfo* pspeechFailInfo =
                pspeechFailTopic->GetMatchingInfo(bdummy, pSpeaker, pPlayer, false, afDisposition, nullptr);
            if (pspeechFailInfo) {
                MenuTopic* pnewTopic;
                BSSimpleList<MenuTopic*>* pnewList;
                BSSimpleList<MenuTopic*>* v18 =
                    static_cast<BSSimpleList<MenuTopic*>*>(ourMalloc(sizeof(BSSimpleList<MenuTopic*>)));
                if (v18) {
                    pnewList = ::new (v18) BSSimpleList<MenuTopic*>;
                }
                else {
                    pnewList = nullptr;
                }
                MenuTopic* apMemory = static_cast<MenuTopic*>(ourMalloc(sizeof(MenuTopic)));
                if (apMemory) {
                    TESObjectREFR* afDispositiona = pSpeaker;
                    TESQuest* OwnerQuest = pspeechFailTopic->GetOwnerQuest(pspeechFailInfo);
                    pnewTopic = ::new (apMemory)
                        MenuTopic(OwnerQuest, pspeechFailTopic, pspeechFailInfo, afDispositiona, false);
                }
                else {
                    pnewTopic = nullptr;
                }
                MenuTopic* pnewTopic_2 = pnewTopic;
                pnewTopic->FirstResponse();
                pSpeechTopic->pInfo->SetSaidOnce();
                if (pSpeechTopic->pInfo->IsSayOnce() || pSpeechTopic->pInfo->IsSayOnceADay()) {
                    if (pSpeechTopic->pInfo->IsSayOnceADay() && pSpeaker->IsMobileObject()) {
                        MobileObject* pmob = static_cast<MobileObject*>(pSpeaker);
                        pmob->AddSayOnceTopicInfo(pSpeechTopic->pInfo);
                    }
                    GetCurrentTopicList()->Remove(pSpeechTopic);
                }
                MenuTopic* pnewTopic_3 = GetGreeting();
                pnewList->AddTail(pnewTopic_3);
                pnewList->AddTail(pnewTopic_2);
                PushTopicList(pnewList);
                FirstTopic(true);
            }
            else {
                Warning(
                    WARN_DIALOGUE,
                    "SpeechChallengeFailure topic had no matching info for this topic; returning to original "
                    "list."
                );
            }
            if (pSpeaker->IsActor()) {
                float iDifficultyMult = GamePlayFormulas::GetSpeechFailPenalty(iChallengeChance);
                static_cast<Actor*>(pSpeaker)->ModifyDispositionTowardActor(pPlayer, iDifficultyMult);
            }
        }
    }

    return bSpeechChallengeLoss;
}

void MenuTopicManager::Update() {
    MenuTopic* CurrentTopic = GetCurrentTopic();
    if ((CurrentTopic) && (CurrentTopic->pInfo)) {
        CurrentTopic->pInfo->AddTopicList();
    }

    if (bSpeechChallengeLoss && (CurrentTopic && !CurrentTopic->bTopicIsChoice)) {
        PopTopicList();
        bSpeechChallengeLoss = false;
    }
    else {
        if ((CurrentTopic) && (!CurrentTopic->bGeneralTopic)) {
            BSSimpleList<TESTopic*>* pTopics;
            bool EntryPoint = CurrentTopic->bTopicIsChoice;
            if (EntryPoint) {
                pTopics = &CurrentTopic->pInfo->GetConversationData()->m_listLinkTo;
            }
            else {
                pTopics = pPlayer->GetTopicList();
            }
            TESTopic* apPreviousTopic = CurrentTopic->pTopic;
            if (pTopics) {
                BSSimpleList<MenuTopic*>* CurrentTopicList = GetCurrentTopicList();
                ClearTopicList(CurrentTopicList, false);
                FillTopicList(pTopics, false, EntryPoint, apPreviousTopic);
            }
            if (bSpeechChallengeLoss) {
                bSpeechChallengeLoss = false;
            }
        }
    }
}

void MenuTopicManager::FillTopicList(
    BSSimpleList<TESTopic*>* apList,
    bool abPersuasion,
    bool abIsChoiceList,
    TESTopic* apPreviousTopic
) {
    bool bDispositionFailure;
    bool bgeneral = false;
    Actor* pactor = dynamic_cast<Actor*>(pSpeaker);
    while (apList && apList->m_item) {
        TESTopic* pTopic = apList->m_item;
        apList = apList->m_pkNext;
        if ((!abPersuasion || pTopic->IsPersuasion()) && (abPersuasion || !pTopic->IsPersuasion())) {
            DUMMY_NODE_STATE DummyFlag = pTopic->GetDummyNodeState();
            int32_t v5 = pPlayer->GetActorValue(ActorValue::INTELLIGENCE);
            if (v5 > iDialogueDummySpeakThisIntOrBelow.Int()) {
                if (DummyFlag != DUM_DUMMY_ONLY) {
                    goto LABEL_12;
                }
            }
            else {
                if (DummyFlag != DUM_NON_DUMMY_ONLY) {
                LABEL_12:;
                    if (pTopic->IsRemappedRumors() && !pactor->HasNoRumors()) {
                        MenuTopic* apitem = nullptr;
                        MenuTopic* pMenuTopic = (&pSpeaker->m_Extra)->GetInfoGeneralTopic(pSpeaker);
                        if ((pMenuTopic) && (!bgeneral)) {
                            if (pMenuTopic->FirstResponse() && !bgeneral &&
                                !GetCurrentTopicList()->IsInList(pMenuTopic)) {
                                GetCurrentTopicList()->AddTail(pMenuTopic);
                            }
                            bgeneral = true;
                        }
                    }
                    bDispositionFailure = false;
                    TESTopicInfo* pInfo = pTopic->GetMatchingInfo(
                        bDispositionFailure, pSpeaker, pPlayer, false, apPreviousTopic, nullptr
                    );
                    if (pInfo) {
                        TESQuest* pQuest = pTopic->GetOwnerQuest(pInfo);
                        bool bSetTopic = false;
                        bool badd = true;
                        if (pTopic->IsRemappedRumors()) {
                            if (bgeneral || pactor->HasNoRumors()) {
                                badd = false;
                            }
                            else {
                                bSetTopic = true;
                            }
                        }
                        MenuTopic* pMenuTopic_2 = nullptr;
                        if (badd) {
                            MenuTopic* pMenuTopic_3;
                            MenuTopic* apMemory = static_cast<MenuTopic*>(ourMalloc(sizeof(MenuTopic)));
                            if (apMemory) {
                                pMenuTopic_3 =
                                    ::new (apMemory) MenuTopic(pQuest, pTopic, pInfo, pSpeaker, bDispositionFailure);
                            }
                            else {
                                pMenuTopic_3 = nullptr;
                            }
                            pMenuTopic_2 = pMenuTopic_3;
                        }
                        if (badd && (pMenuTopic_2->FirstResponse() && !GetCurrentTopicList()->IsInList(pMenuTopic_2))) {
                            GetCurrentTopicList()->AddTail(pMenuTopic_2);
                        }
                        else {
                            if (pMenuTopic_2) {
                                delete pMenuTopic_2;
                                pMenuTopic_2 = nullptr;
                            }
                        }
                        if ((bSetTopic) && (pMenuTopic_2) && (!pMenuTopic_2->pInfo->IsSayOnce())) {
                            (&pSpeaker->m_Extra)->SetInfoGeneralTopic(pMenuTopic_2);
                            pSpeaker->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                        }
                    }
                }
            }
        }
    }
    if (!abIsChoiceList) {
        GetCurrentTopicList()->Sort(&MenuTopicManager::PrioritySort);
    }
}

void* MenuTopicManager::PopTopicList() {
    if (listTopicLists.m_pkNext) {
        BSSimpleList<MenuTopic*>* pList = listTopicLists.m_item;
        if (pList) {
            listTopicLists.RemoveHead();
            ClearTopicList(pList, false);
            pList->RemoveAllPointerList();
            delete pList;
        }
    }
    return listTopicLists.m_item;
}

// Destroys and frees the manager singleton and clears it.
void MenuTopicManager::Destroy() {
    if (pInstance) {
        MenuTopicManager* pManager = pInstance;
        if (pManager) {
            delete pManager;
        }
        pInstance = nullptr;
    }
}

MenuTopicManager* MenuTopicManager::Instance() {
    MenuTopicManager* v1;
    if (MenuTopicManager::pInstance) {
        return MenuTopicManager::pInstance;
    }
    void* v2 = ourMalloc(sizeof(MenuTopicManager));
    if (v2) {
        v1 = ::new (v2) MenuTopicManager;
    }
    else {
        v1 = nullptr;
    }
    MenuTopicManager::pInstance = v1;
    return MenuTopicManager::pInstance;
}

void MenuTopicManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t MenuTopicManager::PrioritySort(MenuTopic* apTopic1, MenuTopic* apTopic2) {
    TESTopic* ProcessLevel = apTopic1->GetTopic();
    double DataFogFarPlane = ProcessLevel->GetPriority();
    TESTopic* v3 = apTopic2->GetTopic();
    if (v3->GetPriority() < DataFogFarPlane) {
        return -1;
    }
    TESTopic* v4 = apTopic1->GetTopic();
    double v7 = v4->GetPriority();
    TESTopic* v5 = apTopic2->GetTopic();
    return v5->GetPriority() > v7;
}

DUMMY_NODE_STATE TESTopic::GetDummyNodeState() {
    if (m_Data.cFlags & 0x10) {
        return DUM_DUMMY_ONLY;
    }
    return static_cast<DUMMY_NODE_STATE>(((m_Data.cFlags & 0x20) != 0) + DUM_EITHER);
}
