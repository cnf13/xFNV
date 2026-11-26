#include "Fallout/AI/DialoguePackage.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

void DialoguePackage::LoadGame() {
    TESPackage::LoadGame();
    uint16_t usBlockSize = 0;
    uint8_t* pStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockHeader;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        if (uiBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                    569,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                    569,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    LoadGameDataOLD(&flistentimer, sizeof(flistentimer));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        LoadGameDataOLD(&bWaitingforLipFile, sizeof(bWaitingforLipFile));
    }
    uint32_t uiTalkingActorID;
    LoadNumericID(&uiTalkingActorID, 4);
    pTalkingActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(uiTalkingActorID));
    uint32_t uiActorID;
    LoadNumericID(&uiActorID, 4);
    pActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(uiActorID));
    uint32_t uiTargetActorID;
    LoadNumericID(&uiTargetActorID, 4);
    pTargetActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(uiTargetActorID));
    uint32_t uiTopicID;
    LoadNumericID(&uiTopicID, 4);
    TESForm* pTopicForm = TESForm::GetFormByNumericID(uiTopicID);
    ptopic = dynamic_cast<TESTopic*>(pTopicForm);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
        int16_t sHasConversation;
        LoadGameDataOLD(&sHasConversation, sizeof(sHasConversation));
        if (sHasConversation) {
            pConversation = new Conversation;
            pConversation->LoadGame();
            int16_t sItemIndex;
            LoadGameDataOLD(&sItemIndex, sizeof(sItemIndex));
            if (sItemIndex == -1) {
                pCurrentItem = nullptr;
            }
            else {
                pCurrentItem = pConversation->GetDialogueItemByIndex(sItemIndex);
            }
            int16_t sResponseIndex;
            LoadGameDataOLD(&sResponseIndex, sizeof(sResponseIndex));
            // Stores the sign-extended response index in the field before it resolves it.
            pCurrentResponse = reinterpret_cast<DialogueResponse*>(static_cast<intptr_t>(sResponseIndex));
            if (!pCurrentItem || sResponseIndex == -1) {
                pCurrentResponse = nullptr;
            }
            else {
                pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(sResponseIndex);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
        Conversation* pSavedConversation;
        // The save keeps 4 bytes: they fill the low half of the zeroed pointer.
        pSavedConversation = nullptr;
        LoadGameDataOLD(&pSavedConversation, 4);
        pConversation = pSavedConversation;
        if (pSavedConversation) {
            // Old saves: the item and response indices stay in the pointer fields, sign-extended, until
            // InitLoadGame resolves them.
            int16_t sItemIndex;
            LoadGameDataOLD(&sItemIndex, sizeof(sItemIndex));
            pCurrentItem = reinterpret_cast<DialogueItem*>(static_cast<intptr_t>(sItemIndex));
            int16_t sResponseIndex;
            LoadGameDataOLD(&sResponseIndex, sizeof(sResponseIndex));
            pCurrentResponse = reinterpret_cast<DialogueResponse*>(static_cast<intptr_t>(sResponseIndex));
        }
        else {
            pCurrentItem = nullptr;
            pCurrentResponse = nullptr;
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
                if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                    uint32_t uiFlags = pHeader->iFlags;
                    int32_t iVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&pStartBufferPosition[usBlockSize] - pCurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                        648,
                        pHeader->iFormID,
                        pEditorID,
                        iVersion,
                        uiFlags
                    );
                }
            }
            else {
                uint32_t uiFlags = pHeader->iFlags;
                int32_t iVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pCurrentBufferPosition - &pStartBufferPosition[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                    648,
                    pHeader->iFormID,
                    pEditorID,
                    iVersion,
                    uiFlags
                );
            }
        }
        else if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
            if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(&pStartBufferPosition[usBlockSize] - pCurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                    648,
                    iVersion
                );
            }
        }
        else {
            int32_t iVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pCurrentBufferPosition - &pStartBufferPosition[usBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                648,
                iVersion
            );
        }
    }
}

DialoguePackage::DialoguePackage()
  : ptopic(nullptr), flistentimer(0.0f), pTalkingActor(nullptr), bWaitingforLipFile(false), pTargetLocation(nullptr),
    bUseIdleManager(false), pConversation(nullptr), pCurrentItem(nullptr), pCurrentResponse(nullptr), pActor(nullptr),
    pTargetActor(nullptr), pTalkingActivator(nullptr), bActorHeadTrack(true), bCreatedPack(false),
    bContinueMoving(false), bTargetContinueMoving(false), bTargetMoveTowardStarter(false),
    bDoNotControlTargetMovement(false), bReachedAmbushLocation(false), iEmotion(0), iEmotionValue(50),
    bSoundFile(true) {}

DialoguePackage::DialoguePackage(
    void* apConversation,
    Actor* apAct,
    MobileObject* apTarg,
    Actor* apTalkActivator,
    bool abCreated,
    bool abUseIdleManager
)
  : ptopic(nullptr), flistentimer(0.0f), pTalkingActor(nullptr), bWaitingforLipFile(false), pTargetLocation(nullptr),
    bUseIdleManager(abUseIdleManager), pConversation(static_cast<Conversation*>(apConversation)), pCurrentItem(nullptr),
    pCurrentResponse(nullptr), pActor(apAct), pTargetActor(apTarg), pTalkingActivator(apTalkActivator),
    bActorHeadTrack(true), bCreatedPack(abCreated), bContinueMoving(false), bTargetContinueMoving(false),
    bTargetMoveTowardStarter(false), bDoNotControlTargetMovement(false), bReachedAmbushLocation(false), iEmotion(0),
    iEmotionValue(50), bSoundFile(true) {
    if (pConversation) {
        pConversation->FirstItem();
    }
    pActor->SetActionComplete(false);
    pPlayer->CheckForClosesConversation(apAct, apTarg, this);
}

DialoguePackage::~DialoguePackage() {
    if (this == pPlayer->pClosestConversation) {
        pPlayer->pClosestConversation = nullptr;
    }
    if (this == pPlayer->pAIConversationRunning) {
        pPlayer->pAIConversationRunning = nullptr;
    }
    if (pConversation) {
        delete pConversation;
    }
}

void DialoguePackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pTalkingActor, 0);
    apSaveGameBuffer->SaveFormID(pActor, 0);
    apSaveGameBuffer->SaveFormID(pTargetActor, 0);
    apSaveGameBuffer->SaveFormID(pTalkingActivator, 0);
    apSaveGameBuffer->SaveFormID(pTargetLocation, 0);
    apSaveGameBuffer->SaveFormID(ptopic, 0);
    apSaveGameBuffer->SaveDataEndian(&flistentimer, sizeof(flistentimer), 0);
    apSaveGameBuffer->SaveDataEndian(&bWaitingforLipFile, sizeof(bWaitingforLipFile), 0);
    apSaveGameBuffer->SaveDataEndian(&bActorHeadTrack, sizeof(bActorHeadTrack), 0);
    apSaveGameBuffer->SaveDataEndian(&bCreatedPack, sizeof(bCreatedPack), 0);
    apSaveGameBuffer->SaveDataEndian(&bContinueMoving, sizeof(bContinueMoving), 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetContinueMoving, sizeof(bTargetContinueMoving), 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetMoveTowardStarter, sizeof(bTargetMoveTowardStarter), 0);
    apSaveGameBuffer->SaveDataEndian(&bDoNotControlTargetMovement, sizeof(bDoNotControlTargetMovement), 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotion, sizeof(iEmotion), 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotionValue, sizeof(iEmotionValue), 0);
    apSaveGameBuffer->SaveDataEndian(&bUseIdleManager, sizeof(bUseIdleManager), 0);
    apSaveGameBuffer->SaveDataEndian(&bSoundFile, sizeof(bSoundFile), 0);
    apSaveGameBuffer->SaveDataEndian(&bReachedAmbushLocation, sizeof(bReachedAmbushLocation), 0);
    char cHasConversation = pConversation != nullptr;
    apSaveGameBuffer->SaveDataEndian(&cHasConversation, sizeof(cHasConversation), 0);
    if (cHasConversation) {
        pConversation->SaveGame(apSaveGameBuffer);
        int16_t sCurrentItemIndex;
        if (pCurrentItem) {
            sCurrentItemIndex = pConversation->GetDialogueItemIndex(pCurrentItem);
        }
        else {
            sCurrentItemIndex = -1;
        }
        apSaveGameBuffer->SaveDataEndian(&sCurrentItemIndex, sizeof(sCurrentItemIndex), 0);
        int16_t sCurrentResponseIndex;
        if (pCurrentItem && pCurrentResponse) {
            sCurrentResponseIndex = pCurrentItem->GetDialogueResponseIndex(pCurrentResponse);
        }
        else {
            sCurrentResponseIndex = -1;
        }
        apSaveGameBuffer->SaveDataEndian(&sCurrentResponseIndex, sizeof(sCurrentResponseIndex), 0);
    }
}

void DialoguePackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pTalkingActor);
    apLoadGameBuffer->LoadFormID(pActor);
    apLoadGameBuffer->LoadFormID(pTargetActor);
    apLoadGameBuffer->LoadFormID(pTalkingActivator);
    apLoadGameBuffer->LoadFormID(pTargetLocation);
    uint32_t uiTopicID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiTopicID);
    ptopic = dynamic_cast<TESTopic*>(pForm);
    apLoadGameBuffer->LoadData(&flistentimer, sizeof(flistentimer));
    apLoadGameBuffer->LoadData(&bWaitingforLipFile, sizeof(bWaitingforLipFile));
    apLoadGameBuffer->LoadData(&bActorHeadTrack, sizeof(bActorHeadTrack));
    apLoadGameBuffer->LoadData(&bCreatedPack, sizeof(bCreatedPack));
    apLoadGameBuffer->LoadData(&bContinueMoving, sizeof(bContinueMoving));
    apLoadGameBuffer->LoadData(&bTargetContinueMoving, sizeof(bTargetContinueMoving));
    apLoadGameBuffer->LoadData(&bTargetMoveTowardStarter, sizeof(bTargetMoveTowardStarter));
    apLoadGameBuffer->LoadData(&bDoNotControlTargetMovement, sizeof(bDoNotControlTargetMovement));
    apLoadGameBuffer->LoadData(&iEmotion, sizeof(iEmotion));
    apLoadGameBuffer->LoadData(&iEmotionValue, sizeof(iEmotionValue));
    apLoadGameBuffer->LoadData(&bUseIdleManager, sizeof(bUseIdleManager));
    apLoadGameBuffer->LoadData(&bSoundFile, sizeof(bSoundFile));
    if (apLoadGameBuffer->GetVersion() >= 3u) {
        apLoadGameBuffer->LoadData(&bReachedAmbushLocation, sizeof(bReachedAmbushLocation));
    }
    bool bHasConversation = false;
    apLoadGameBuffer->LoadData(&bHasConversation, sizeof(bHasConversation));
    if (bHasConversation) {
        pConversation = new Conversation;
        pConversation->LoadGame(apLoadGameBuffer);
        int16_t sItemIndex = -1;
        apLoadGameBuffer->LoadData(&sItemIndex, sizeof(sItemIndex));
        if (sItemIndex != -1) {
            pCurrentItem = pConversation->GetDialogueItemByIndex(sItemIndex);
        }
        int16_t sResponseIndex = -1;
        apLoadGameBuffer->LoadData(&sResponseIndex, sizeof(sResponseIndex));
        if (pCurrentItem && sResponseIndex != -1) {
            pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(sResponseIndex);
        }
    }
}

void DialoguePackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
    MobileObject* pResolvedTalker = nullptr;
    if (pTalkingActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActor)));
        pResolvedTalker = dynamic_cast<MobileObject*>(pForm);
    }
    pTalkingActor = pResolvedTalker;
    MobileObject* pResolvedActor = nullptr;
    if (pActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        pResolvedActor = dynamic_cast<MobileObject*>(pForm);
    }
    pActor = pResolvedActor;
    MobileObject* pResolvedTarget = nullptr;
    if (pTargetActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetActor)));
        pResolvedTarget = dynamic_cast<MobileObject*>(pForm);
    }
    pTargetActor = pResolvedTarget;
    MobileObject* pResolvedActivator = nullptr;
    if (pTalkingActivator) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActivator))
        );
        pResolvedActivator = dynamic_cast<MobileObject*>(pForm);
    }
    pTalkingActivator = pResolvedActivator;
    TESObjectREFR* pResolvedLocation = nullptr;
    if (pTargetLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetLocation))
        );
        pResolvedLocation = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTargetLocation = pResolvedLocation;
    if (pConversation) {
        pConversation->InitItem(apLoadGameBuffer);
    }
}

void DialoguePackage::Copy(TESForm* apCopy) {
    TESPackage::Copy(apCopy);
    dynamic_cast<DialoguePackage*>(apCopy);
}

uint16_t DialoguePackage::GetSaveSize() {
    uint16_t usSaveSize = TESPackage::GetSaveSize();
    uint16_t usBaseSize = usSaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSaveSize += 6;
    }
    int16_t sDataSize = usSaveSize + 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        ++sDataSize;
    }
    uint16_t usSize = sDataSize + 18;
    if (pConversation) {
        usSize += pConversation->GetSaveSize() + 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usBaseSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                500,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBaseSize,
                500,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
    }
    return usSize;
}

void DialoguePackage::SaveGame() {
    TESPackage::SaveGame();
    uint16_t usBlockSize = 0;
    uint8_t* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    SaveGameDataOLD(&flistentimer, sizeof(flistentimer));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        SaveGameDataOLD(&bWaitingforLipFile, sizeof(bWaitingforLipFile));
    }
    uint32_t uiTalkingActorID = 0;
    if (pTalkingActor) {
        uiTalkingActorID = pTalkingActor->iFormID;
    }
    SaveNumericID(&uiTalkingActorID, 4);
    uint32_t uiActorID = 0;
    if (pActor) {
        uiActorID = pActor->iFormID;
    }
    SaveNumericID(&uiActorID, 4);
    uint32_t uiTargetActorID = 0;
    if (pTargetActor) {
        uiTargetActorID = pTargetActor->iFormID;
    }
    SaveNumericID(&uiTargetActorID, 4);
    uint32_t uiTopicID = 0;
    if (ptopic) {
        uiTopicID = ptopic->iFormID;
    }
    SaveNumericID(&uiTopicID, 4);
    uint16_t usSize = 0;
    if (pConversation) {
        usSize = pConversation->GetSaveSize();
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&usSize, sizeof(usSize));
    if (usSize) {
        pConversation->SaveGame();
        int16_t sCurrentItemIndex = -1;
        if (pCurrentItem) {
            sCurrentItemIndex = pConversation->GetDialogueItemIndex(pCurrentItem);
        }
        SaveGameDataOLD(&sCurrentItemIndex, sizeof(sCurrentItemIndex));
        int16_t sCurrentResponseIndex = -1;
        if (pCurrentItem && pCurrentResponse) {
            sCurrentResponseIndex = pCurrentItem->GetDialogueResponseIndex(pCurrentResponse);
        }

        SaveGameDataOLD(&sCurrentResponseIndex, sizeof(sCurrentResponseIndex));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t uiFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                uiFlags,
                561,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                561,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pBlockEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = pBlockSize;
        if (pBlockEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp",
                561
            );
        }
        // The save buffer is raw bytes: the 2-byte size placeholder is patched in place.
        *reinterpret_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pBlockEnd - pBlockStart);
    }
}

void DialoguePackage::InitLoadGame() {
    TESPackage::InitLoadGame();
    TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActor)));
    pTalkingActor = dynamic_cast<Actor*>(pForm);
    pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
    pActor = dynamic_cast<Actor*>(pForm);
    pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetActor)));
    pTargetActor = dynamic_cast<Actor*>(pForm);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u && pConversation) {
        pConversation->InitLoadGame();
    }

    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
        // Old saves: LoadGame left the item and response indices in the pointer fields; their low 16 bits are read.
        Conversation* pOldConversation = pConversation;
        int16_t sCurrentItemIndex = static_cast<int16_t>(reinterpret_cast<uintptr_t>(pCurrentItem));
        int16_t sCurrentResponseIndex = static_cast<int16_t>(reinterpret_cast<uintptr_t>(pCurrentResponse));
        if (pOldConversation) {
            pConversation = new Conversation(pActor, pTargetActor, ptopic);
            if (sCurrentItemIndex == -1) {
                pCurrentItem = nullptr;
            }
            else {
                pCurrentItem = pConversation->GetDialogueItemByIndex(sCurrentItemIndex);
            }
            if (!pCurrentItem || sCurrentResponseIndex == -1) {
                pCurrentResponse = nullptr;
            }
            else {
                pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(sCurrentResponseIndex);
            }
            pConversation->SetCurrentItem(pCurrentItem);
            pCurrentItem->SetCurrentResponse(pCurrentResponse);
        }
    }
}

bool DialoguePackage::IsPackageOwner(MobileObject* apOwner) {
    return static_cast<const void*>(apOwner) == static_cast<const void*>(pActor);
}

void DialoguePackage::DeleteSoundHandle() {
    if (pTalkingActor) {
        pTalkingActor->StopCurrentDialogue();
    }
}

bool DialoguePackage::GetContinueMoving() {
    return bContinueMoving;
}

void DialoguePackage::SetContinueMoving(bool abContinueMoving) {
    bContinueMoving = abContinueMoving;
}

void DialoguePackage::SetTargetContinueMoving(bool abMove) {
    bTargetContinueMoving = abMove;
}

void DialoguePackage::SetTargetMoveTowardStarter(bool abMove) {
    bTargetMoveTowardStarter = abMove;
}

bool DialoguePackage::IsCreated() {
    return bCreatedPack;
}

bool DialoguePackage::GetTargetContinueMoving() {
    return bTargetContinueMoving;
}

bool DialoguePackage::GetTargetMoveTowardStarter() {
    return bTargetMoveTowardStarter;
}

void DialoguePackage::SetReachedAmbushLocation(bool abFlag) {
    bReachedAmbushLocation = abFlag;
}

bool DialoguePackage::GetReachedAmbushLocation() {
    return bReachedAmbushLocation;
}

bool DialoguePackage::GetShouldHeadTrackTarget() {
    return bActorHeadTrack;
}

MobileObject* DialoguePackage::GetActorStartedConversation() {
    return pActor;
}

MobileObject* DialoguePackage::GetTargetOfConversation() {
    return pTargetActor;
}

// The completion callbacks take the speaker's form ID as their untyped context: the integer is passed in the
// pointer-sized context argument, hence the integer-to-pointer casts below.
void DialoguePackage::Speak(bool abUseSound) {
    bool bCheckforIdle = true;
    if (pTalkingActor && !pTalkingActor->bVoiceFileDone) {
        return;
    }
    if (pActor->pCurrentProcess->GetCurrentActionComplete()) {
        return;
    }
    if (!bWaitingforLipFile) {
        if (!pCurrentItem) {
            if (!pConversation) {
                pActor->SetActionComplete(true);
                pTargetActor->SetActionComplete(true);
                return;
            }
            if (GetShouldHeadTrackTarget() && !bDoNotControlTargetMovement) {
                pTargetActor->pCurrentProcess->ClearActionHeadTrackTarget(true);
                pTargetActor->pCurrentProcess->SetDialogHeadTrackTarget(pActor);
            }
            pConversation->FirstItem();
            pCurrentItem = pConversation->GetCurrentItem();
            if (pCurrentItem) {
                pCurrentItem->FirstResponse();
                pCurrentResponse = pCurrentItem->GetCurrentResponse();
            }
            else {
                pActor->SetActionComplete(true);
                if (!GetDialogueDoNotControlTarget()) {
                    pTargetActor->SetActionComplete(true);
                }
            }
        }
        else if (pCurrentResponse) {
            pCurrentItem->NextResponse();
            pCurrentResponse = pCurrentItem->GetCurrentResponse();
        }
        if (pActor) {
            if (pActor->IsDying() || pActor->IsDead(false)) {
                if (!pCurrentResponse) {
                    pCurrentItem = nullptr;
                }
            }
            else if (!pCurrentResponse) {
                pConversation->GetNext();
                pCurrentItem = pConversation->GetCurrentItem();
            }
        }
    }
    if (!pCurrentItem) {
        pActor->SetActionComplete(true);
        if (!GetDialogueDoNotControlTarget()) {
            pTargetActor->SetActionComplete(true);
        }
        return;
    }
    if (!pCurrentResponse) {
        pCurrentItem->FirstResponse();
        pCurrentResponse = pCurrentItem->GetCurrentResponse();
        if (!pCurrentResponse) {
            Warning("AI: No responses found for conversation Topic Info (%08X).", pCurrentItem->pInfo->iFormID);
        }
    }
    if (!pCurrentResponse) {
        return;
    }
    TESObjectREFR* pSpeaker = pCurrentItem->pSpeaker;
    if (pSpeaker->IsActor()) {
        Actor* pAct = static_cast<Actor*>(pCurrentItem->pSpeaker);
        if (pAct && abUseSound) {
            BSStringT<char> audioFilePath;
            if (pTalkingActor) {
                pTalkingActor->StopCurrentDialogue();
                pTalkingActor = nullptr;
            }
            bool bNoSound = true;
            if (pCurrentResponse->pVoiceSound && bSoundFile) {
                uint32_t uiSoundFileID = pCurrentResponse->pVoiceSound->iFormID;
                BSSoundHandle soundHandle = pAct->PlaySound(
                    uiSoundFileID,
                    false,
                    BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN | BSGameSound::SOUND_FLAG_ONE_SHOT |
                        BSGameSound::SOUND_FLAG_IS_3D,
                    true
                );
                BSAudioManager::QInstance().SetCompletionCallback(
                    soundHandle.iSoundID,
                    &MobileObject::SoundFileCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                );
                bSoundFile = false;
                bNoSound = false;
            }
            if (bNoSound && bSoundFile) {
                audioFilePath.Set(pCurrentResponse->cVoice);
                if (!pAct->pCurrentProcess->GetLipAnim() &&
                    !pAct->pCurrentProcess->GetWaitingforLipFile() &&
                    bBackgroundLoadLipFiles.Bool() &&
                    !pAct->pCurrentProcess->GetLipFileFailed()) {
                    iEmotion = pCurrentResponse->eEmotion;
                    iEmotionValue = pCurrentResponse->uiEmotion;
                    if (pCurrentItem) {
                        pCurrentItem->RunResult(TIRS_BEGIN);
                    }
                    pTalkingActor = pAct;
                    pAct->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                    pAct->SpeakSoundFunction(
                        audioFilePath.String(),
                        handle,
                        pCurrentResponse->eEmotion,
                        pCurrentResponse->uiEmotion,
                        pCurrentResponse->cText.GetLength(),
                        pCurrentResponse->pSpeakerIdle,
                        pCurrentResponse->pListenIdle,
                        pTargetActor,
                        true,
                        false,
                        true,
                        true,
                        bUseIdleManager
                    );
                    if (pTalkingActivator &&
                        static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference())->pTempRef == pAct) {
                        MobileObject* pTarg = pTargetActor;
                        if (pTargetActor == pTalkingActivator) {
                            pTarg = pActor;
                        }
                        pTalkingActor = pTalkingActivator;
                        pTalkingActivator->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                        pTalkingActivator->SpeakSoundFunction(
                            audioFilePath.String(),
                            handle,
                            pCurrentResponse->eEmotion,
                            pCurrentResponse->uiEmotion,
                            pCurrentResponse->cText.GetLength(),
                            pCurrentResponse->pSpeakerIdle,
                            pCurrentResponse->pListenIdle,
                            pTarg,
                            false,
                            false,
                            false,
                            true,
                            true
                        );
                    }
                    if (pAct->pCurrentProcess->GetWaitingforLipFile()) {
                        bWaitingforLipFile = true;
                    }
                    else {
                        BSAudioManager::QInstance().SetCompletionCallback(
                            handle.iSoundID,
                            &MobileObject::DialogueConversationCallBack,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                        );
                        if (pActor != pTargetActor) {
                            flistentimer = 50.0f;
                        }
                        pAct->bVoiceFileDone = false;
                    }
                    return;
                }
                if (pAct->pCurrentProcess->GetWaitingforLipFile() && !pAct->pCurrentProcess->GetLipFileFailed()) {
                    return;
                }
                if (!bBackgroundLoadLipFiles.Bool() && pCurrentItem) {
                    pCurrentItem->RunResult(TIRS_BEGIN);
                }
                pTalkingActor = pAct;
                pAct->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                pAct->SpeakSoundFunction(
                    audioFilePath.String(),
                    handle,
                    pCurrentResponse->eEmotion,
                    pCurrentResponse->uiEmotion,
                    pCurrentResponse->cText.GetLength(),
                    pCurrentResponse->pSpeakerIdle,
                    pCurrentResponse->pListenIdle,
                    pTargetActor,
                    true,
                    false,
                    false,
                    true,
                    bUseIdleManager
                );
                if (pTalkingActivator &&
                    static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference())->pTempRef == pAct) {
                    MobileObject* pTarg = pTargetActor;
                    if (pTargetActor == pTalkingActivator) {
                        pTarg = pActor;
                    }
                    pTalkingActor = pTalkingActivator;
                    pTalkingActivator->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                    pTalkingActivator->SpeakSoundFunction(
                        audioFilePath.String(),
                        handle,
                        pCurrentResponse->eEmotion,
                        pCurrentResponse->uiEmotion,
                        pCurrentResponse->cText.GetLength(),
                        pCurrentResponse->pSpeakerIdle,
                        pCurrentResponse->pListenIdle,
                        pTarg,
                        false,
                        false,
                        false,
                        true,
                        true
                    );
                }
                BSAudioManager::QInstance().SetCompletionCallback(
                    handle.iSoundID,
                    &MobileObject::DialogueConversationCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                );
                pAct->bVoiceFileDone = false;
                bCheckforIdle = false;
                pTalkingActor = pAct;
                pAct->pCurrentProcess->SetLipAnim(nullptr);
                bWaitingforLipFile = false;
                pAct->pCurrentProcess->SetLipFileFailed(false);
                if (pActor != pTargetActor) {
                    flistentimer = 2.0f;
                }
                // Asks both actors for their animation and deliberately keeps neither result.
                pTalkingActor->GetAnimation();
                pTargetActor->GetAnimation();
            }
        }
        if (bCheckforIdle) {
            if (pActor->pCurrentProcess) {
                pActor->pCurrentProcess->SetupSpecialIdle(pActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, false, true, true);
            }
            // Tests the talking actor's process and then idles the target's.
            if (pActor != pTalkingActor && pTalkingActor && pTalkingActor->pCurrentProcess) {
                pTargetActor->pCurrentProcess
                    ->SetupSpecialIdle(pTargetActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, false, true, true);
            }
        }
        return;
    }
    if (pSpeaker && abUseSound) {
        BSStringT<char> audioFilePath;
        BSStringT<char> tempstring(pCurrentResponse->cVoice);
        if (pCurrentResponse->cVoice.String()) {
            char acAudioFile[512];
            BSstrcpy(acAudioFile, 512, pCurrentResponse->cVoice.String());
            if (pTalkingActor) {
                pTalkingActor->StopCurrentDialogue();
                pTalkingActor = nullptr;
            }
            audioFilePath.Set(pCurrentResponse->cVoice);
            if (pTalkingActivator) {
                pTalkingActivator->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                pTalkingActor = pTalkingActivator;
                pTalkingActivator->SpeakSoundFunction(
                    audioFilePath.String(),
                    handle,
                    pCurrentResponse->eEmotion,
                    pCurrentResponse->uiEmotion,
                    pCurrentResponse->cText.GetLength(),
                    pCurrentResponse->pSpeakerIdle,
                    pCurrentResponse->pListenIdle,
                    pTargetActor,
                    false,
                    false,
                    false,
                    true,
                    true
                );
                BSAudioManager::QInstance().SetCompletionCallback(
                    handle.iSoundID,
                    &MobileObject::DialogueConversationCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pTalkingActivator->iFormID))
                );
                pTalkingActivator->bVoiceFileDone = false;
            }
            else {
                uint32_t audioFlags = BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_VOICE |
                    BSGameSound::SOUND_FLAG_IS_3D;
                handle = BSAudio::QInstance().GetSoundHandleByFilename(acAudioFile, audioFlags, nullptr);
                NiPoint3 kSpeakerLoc = pSpeaker->GetLocationOnReference();
                // Reads the maximum distance first.
                float fMaxDist = BSAudio::GetMaxDialogDist();
                float fMinDist = BSAudio::GetMinDialogDist();
                handle.SetMinMax(fMinDist, fMaxDist);
                handle.SetPosition(kSpeakerLoc.x, kSpeakerLoc.y, kSpeakerLoc.z);
                handle.SetObjectToFollow(pSpeaker->Get3D());
                handle.PlayAfter(1, 0);
            }
        }
        if ((this == pPlayer->pClosestConversation || ReferenceProcessLists.bShowSubtitle) &&
            (pConversation->GetCurrentItem() &&
             pConversation->GetCurrentItem()->GetCurrentResponse() &&
             pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String()) &&
            (bGeneralSubtitles.Bool() ||
             (pSpeaker->IsActor() && static_cast<uint8_t>(static_cast<Actor*>(pSpeaker)->GetShowSubtitles()))))

        // GetShowSubtitles: only the low byte of the result is tested.
        {
            // Copies the location, then reads the text.
            NiPoint3 kSpeakerLoc = pSpeaker->GetLocationOnReference();
            const char* pText = pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String();
            Interface::ShowSubtitle(pText, handle, kSpeakerLoc, pTargetActor, false);
        }

        if (flistentimer <= 0.0 && pTargetActor != pSpeaker) {
            flistentimer = 10.0f;
        }
    }
}
