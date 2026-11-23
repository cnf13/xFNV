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
    return lResponses.ItemsInList() == 0;
}

void MenuTopic::FillResponseList(TESQuest* apQuest, TESTopic* apTopic, TESTopicInfo* apInfo, TESObjectREFR* apSpeaker) {
    // ResponseListWrapper has no C++ destructor.
    ResponseListWrapper lResponseList;
    pInfo->CloneResponseList(&lResponseList);
    TESResponse* presponse = lResponseList.pHead;
    int32_t i = 0;
    while (presponse) {
        if (presponse->GetResponseTextLength() && (!bGeneralTopic || !i)) {
            DialogueResponse* pMenuResponse;
            pMenuResponse = new DialogueResponse(apQuest, apTopic, apInfo, apSpeaker, presponse);
            lResponses.AddTail(pMenuResponse);
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
        pSaveLoadGameOLD->LoadGameDataOLD(&uiMarker, sizeof(uiMarker));
        if (uiMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint8_t ucVersion = pHeader->cVersion;
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
                    773,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    ucVersion,
                    uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
                    773,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usSize, sizeof(usSize));
    }
    char acText[0x104];
    ourMemset(acText, 0, 0x104);
    uint8_t ucLength;
    pSaveLoadGameOLD->LoadGameDataOLD(&ucLength, sizeof(ucLength));
    if (ucLength) {
        pSaveLoadGameOLD->LoadGameDataOLD(acText, ucLength);
        cTopicText.Set(acText, 0);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&bTopicIsChoice, sizeof(bTopicIsChoice));
    pSaveLoadGameOLD->LoadGameDataOLD(&bGeneralTopic, sizeof(bGeneralTopic));
    pSaveLoadGameOLD->LoadGameDataOLD(&bTopicIsNew, sizeof(bTopicIsNew));
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
                809,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                ucVersion,
                uiFlags
            );
        }
        else if (pNow < pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>((pStart + usSize) - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
                809,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                ucVersion,
                uiFlags
            );
        }
    }
    else if (pNow > pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>(pNow - (pStart + usSize)),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
            809,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
    else if (pNow < pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>((pStart + usSize) - pNow),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
            809,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
}

uint16_t MenuTopic::GetSaveSize() {
    int16_t iblockHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeaderSize = 6;
    }
    uint16_t isize = BSstrlen(GetTopicText()) + (iblockHeaderSize + 1) + 15;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                729,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                729,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
    }
    return isize;
}

void MenuTopic::SaveGame() {
    uint32_t iblockHeader;
    int16_t iblockSizePlaceholder[3];
    uint8_t clength;
    uint32_t itopicID;
    uint32_t iinfoID;
    uint32_t iquestID;
    iblockSizePlaceholder[0] = 0;
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pblockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(iblockSizePlaceholder, sizeof(*iblockSizePlaceholder));
    }
    const char* ptext = GetTopicText();
    clength = BSstrlen(ptext);
    pSaveLoadGameOLD->SaveGameDataOLD(&clength, sizeof(clength));
    if (clength) {
        pSaveLoadGameOLD->SaveGameDataOLD(ptext, clength);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&bTopicIsChoice, sizeof(bTopicIsChoice));
    pSaveLoadGameOLD->SaveGameDataOLD(&bGeneralTopic, sizeof(bGeneralTopic));
    pSaveLoadGameOLD->SaveGameDataOLD(&bTopicIsNew, sizeof(bTopicIsNew));
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
        uint8_t* pbufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pcEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pbufferPosition - itempStartPosition,
                pheader->iFormID,
                pcEditorID,
                iFlags,
                767,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pbufferPosition - itempStartPosition,
                767,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Dialogue\\MenuTopic.cpp",
                767
            );
        }
        *pblockSize = static_cast<uint16_t>(pBuffer - pblockStart);
    }
}

TESTopicInfo* MenuTopic::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    char cText[508];
    apLoadGameBuffer->LoadString(cText);
    cTopicText = cText;
    apLoadGameBuffer->LoadData(&bTopicIsChoice, sizeof(bTopicIsChoice));
    apLoadGameBuffer->LoadData(&bGeneralTopic, sizeof(bGeneralTopic));
    apLoadGameBuffer->LoadData(&bTopicIsNew, sizeof(bTopicIsNew));
    apLoadGameBuffer->LoadData(&bInfoRefusal, sizeof(bInfoRefusal));
    apLoadGameBuffer->LoadData(&bPersuasion, sizeof(bPersuasion));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pQuest = dynamic_cast<TESQuest*>(pForm);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    pTopic = dynamic_cast<TESTopic*>(pForm);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    pOrigTopic = dynamic_cast<TESTopic*>(pForm);
    uiFormID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiFormID);
    TESTopicInfo* pLoadedInfo = dynamic_cast<TESTopicInfo*>(pForm);
    pInfo = pLoadedInfo;
    return pLoadedInfo;
}

void MenuTopic::SaveGame_0(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveString(GetTopicText(), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTopicIsChoice), sizeof(bTopicIsChoice), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bGeneralTopic), sizeof(bGeneralTopic), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTopicIsNew), sizeof(bTopicIsNew), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bInfoRefusal), sizeof(bInfoRefusal), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bPersuasion), sizeof(bPersuasion), 0);
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
    MenuTopic* pTopic = pCurrentTopic->GetItem();
    return pTopic != nullptr;
}

BSSimpleList<MenuTopic*>* MenuTopicManager::GetCurrentTopicList() {
    return listTopicLists.m_item;
}

MenuTopic* MenuTopicManager::GetCurrentTopic() {
    if (pCurrentTopic) {
        MenuTopic* pTopic = pCurrentTopic->GetItem();
        return pTopic;
    }
    return nullptr;
}

// GetInfo is called twice.
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
    int32_t iIndex = 0;
    BSSimpleList<MenuTopic*>* pCurrentTopicList = GetCurrentTopicList();
    if (pCurrentTopicList) {
        pCurrentTopicList = pCurrentTopicList->m_pkNext;
    }
    while (pCurrentTopicList && iIndex < aiIndex) {
        pCurrentTopicList = pCurrentTopicList->m_pkNext;
        ++iIndex;
    }
    if (pCurrentTopicList) {
        pCurrentTopic = pCurrentTopicList;
        if (pCurrentTopic->m_item) {
            return true;
        }
    }
    return false;
}

MenuTopic* MenuTopicManager::GetGreeting() {
    BSSimpleList<MenuTopic*>* pCurrentTopicList = GetCurrentTopicList();
    return pCurrentTopicList->m_item;
}

bool MenuTopicManager::NextTopic() {
    if (pCurrentTopic) {
        pCurrentTopic = pCurrentTopic->m_pkNext;
    }
    return pCurrentTopic && pCurrentTopic->m_item;
}

void MenuTopicManager::ClearTopicList(BSSimpleList<MenuTopic*>* apList, bool abClearGreeting) {
    pCurrentTopic = nullptr;
    BSSimpleList<MenuTopic*>* pList = apList;
    if (!abClearGreeting && apList) {
        pList = apList->m_pkNext;
    }

    while (pList && !pList->IsEmpty()) {
        if (!pList->m_item->bGeneralTopic) {
            MenuTopic* pTopic = pList->m_item;
            if (pTopic) {
                delete pTopic;
            }
        }
        pList->RemoveHead();
    }
}

bool MenuTopicManager::Initialize(TESObjectREFR* apSpeaker, TESTopic* apForceGreeting) {
    BSSimpleList<MenuTopic*>* pList;
    bool bdummyBool;
    ClearData();
    pList = new BSSimpleList<MenuTopic*>;
    listTopicLists.AddHead(pList);
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
            TESQuest* pQuest = pGreetTopic->GetOwnerQuest(pInfo);
            pGreeting = new MenuTopic(pQuest, pGreetTopic, pInfo, pSpeaker, false);
            pGreeting->pTopic->SetPriority(2.1474836e+09f);
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
            WARN_DIALOGUE,
            "No GREETING topic was found for %s. Selecting stock GREETING.",
            pSpeaker->GetFullName()
        );
    }
    FirstTopic(false);
    return pGreeting && pGreeting->pInfo && pGreeting->pInfo->IsGoodbye();
}

bool MenuTopicManager::DoSpeechChallengeCheck(MenuTopic* pSpeechTopic) {
    bool bdummy;
    if (pSpeechTopic && pSpeechTopic->pInfo->IsSpeechChallenge()) {
        float fDisposition;
        if (pSpeaker->IsActor()) {
            fDisposition = static_cast<float>(static_cast<Actor*>(pSpeaker)->GetDisposition(pPlayer, nullptr));
        }
        else {
            fDisposition = fSpeechChallengeDispositionBase.Float();
        }
        uint32_t uiDifficultyMult = pSpeechTopic->pInfo->GetSpeechChallengeDifficultyMult();
        float fSpeech = pPlayer->GetClampedActorFloatValue(ActorValue::SPEECH);
        int32_t iChallengeChance = GamePlayFormulas::GetSpeechChallengeChance(fSpeech, fDisposition, uiDifficultyMult);
        int32_t idieRoll = Random() % 100;
        if (idieRoll < iChallengeChance) {
            int32_t iDifficulty = pSpeechTopic->pInfo->eDifficulty;
            iLastDialogXP = ExperiencePoints::GetExperiencePoints(
                ExperiencePoints::ACTIVITY_SPEECH_CHALLENGE,
                iDifficulty - 1
            );
        }
        else {
            bSpeechChallengeLoss = true;
            MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
            TESTopic* pspeechFailTopic = TESTopic::GetTopic(DT_TOPIC, 11);
            TESTopic* pchallengeTopic = pSpeechTopic->pTopic;
            TESTopicInfo* pspeechFailInfo = pspeechFailTopic->GetMatchingInfo(
                bdummy,
                pSpeaker,
                pPlayer,
                false,
                pchallengeTopic,
                nullptr
            );
            if (pspeechFailInfo) {
                BSSimpleList<MenuTopic*>* pnewList = new BSSimpleList<MenuTopic*>;
                MenuTopic* pnewTopic = new MenuTopic(
                    pspeechFailTopic->GetOwnerQuest(pspeechFailInfo),
                    pspeechFailTopic,
                    pspeechFailInfo,
                    pSpeaker,
                    false
                );
                pnewTopic->FirstResponse();
                pSpeechTopic->pInfo->SetSaidOnce();
                if (pSpeechTopic->pInfo->IsSayOnce() || pSpeechTopic->pInfo->IsSayOnceADay()) {
                    if (pSpeechTopic->pInfo->IsSayOnceADay() && pSpeaker->IsMobileObject()) {
                        MobileObject* pmob = static_cast<MobileObject*>(pSpeaker);
                        pmob->AddSayOnceTopicInfo(pSpeechTopic->pInfo);
                    }
                    GetCurrentTopicList()->Remove(pSpeechTopic);
                }
                MenuTopic* pgreeting = GetGreeting();
                pnewList->AddTail(pgreeting);
                pnewList->AddTail(pnewTopic);
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
                float fDifficultyMult = GamePlayFormulas::GetSpeechFailPenalty(iChallengeChance);
                static_cast<Actor*>(pSpeaker)->ModifyDispositionTowardActor(pPlayer, fDifficultyMult);
            }
        }
    }

    return bSpeechChallengeLoss;
}

void MenuTopicManager::Update() {
    MenuTopic* pCurrentTopic = GetCurrentTopic();
    if (pCurrentTopic && pCurrentTopic->pInfo) {
        pCurrentTopic->pInfo->AddTopicList();
    }

    if (bSpeechChallengeLoss && pCurrentTopic && !pCurrentTopic->bTopicIsChoice) {
        PopTopicList();
        bSpeechChallengeLoss = false;
    }
    else if (pCurrentTopic && !pCurrentTopic->bGeneralTopic) {
        BSSimpleList<TESTopic*>* pTopics;
        bool bIsChoice = pCurrentTopic->bTopicIsChoice;
        if (bIsChoice) {
            pTopics = &pCurrentTopic->pInfo->GetConversationData()->m_listLinkTo;
        }
        else {
            pTopics = pPlayer->GetTopicList();
        }
        TESTopic* pPreviousTopic = pCurrentTopic->pTopic;
        if (pTopics) {
            ClearTopicList(GetCurrentTopicList(), false);
            FillTopicList(pTopics, false, bIsChoice, pPreviousTopic);
        }
        if (bSpeechChallengeLoss) {
            bSpeechChallengeLoss = false;
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
            DUMMY_NODE_STATE eDummyState = pTopic->GetDummyNodeState();
            int32_t iIntelligence = pPlayer->GetActorValue(ActorValue::INTELLIGENCE);
            if (iIntelligence > iDialogueDummySpeakThisIntOrBelow.Int() ? eDummyState != DUM_DUMMY_ONLY
                                                                        : eDummyState != DUM_NON_DUMMY_ONLY) {
                if (pTopic->IsRemappedRumors() && !pactor->HasNoRumors()) {
                    MenuTopic* pMenuTopic = pSpeaker->m_Extra.GetInfoGeneralTopic(pSpeaker);
                    if (pMenuTopic && !bgeneral) {
                        if (pMenuTopic->FirstResponse() && !bgeneral && !GetCurrentTopicList()->IsInList(pMenuTopic)) {
                            GetCurrentTopicList()->AddTail(pMenuTopic);
                        }
                        bgeneral = true;
                    }
                }
                bDispositionFailure = false;
                TESTopicInfo* pInfo = pTopic->GetMatchingInfo(
                    bDispositionFailure,
                    pSpeaker,
                    pPlayer,
                    false,
                    apPreviousTopic,
                    nullptr
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
                    MenuTopic* pMenuTopic = nullptr;
                    if (badd) {
                        pMenuTopic = new MenuTopic(pQuest, pTopic, pInfo, pSpeaker, bDispositionFailure);
                    }
                    if (badd && pMenuTopic->FirstResponse() && !GetCurrentTopicList()->IsInList(pMenuTopic)) {
                        GetCurrentTopicList()->AddTail(pMenuTopic);
                    }
                    else if (pMenuTopic) {
                        delete pMenuTopic;
                        pMenuTopic = nullptr;
                    }
                    if (bSetTopic && pMenuTopic && !pMenuTopic->pInfo->IsSayOnce()) {
                        pSpeaker->m_Extra.SetInfoGeneralTopic(pMenuTopic);
                        pSpeaker->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
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
    MenuTopicManager* pManager;
    if (MenuTopicManager::pInstance) {
        return MenuTopicManager::pInstance;
    }
    pManager = new MenuTopicManager;
    MenuTopicManager::pInstance = pManager;
    return MenuTopicManager::pInstance;
}

void MenuTopicManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t MenuTopicManager::PrioritySort(MenuTopic* apTopic1, MenuTopic* apTopic2) {
    TESTopic* pTopic1 = apTopic1->GetTopic();
    double dPriority1 = pTopic1->GetPriority();
    TESTopic* pTopic2 = apTopic2->GetTopic();
    if (pTopic2->GetPriority() < dPriority1) {
        return -1;
    }
    pTopic1 = apTopic1->GetTopic();
    dPriority1 = pTopic1->GetPriority();
    pTopic2 = apTopic2->GetTopic();
    return pTopic2->GetPriority() > dPriority1;
}

DUMMY_NODE_STATE TESTopic::GetDummyNodeState() {
    if (m_Data.cFlags & DIALOGUE_DATA::FLAG_DUMMY_ONLY) {
        return DUM_DUMMY_ONLY;
    }
    return static_cast<DUMMY_NODE_STATE>(((m_Data.cFlags & DIALOGUE_DATA::FLAG_NON_DUMMY_ONLY) != 0) + DUM_EITHER);
}
