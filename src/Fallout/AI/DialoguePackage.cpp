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
    int16_t v16;
    int16_t v17;
    Conversation* v18;
    int16_t v19;
    int16_t apBuffer;
    int16_t apBuffer_4;
    uint32_t iblockHeader;
    uint32_t itargetActorID__icreationTime;
    uint16_t iblockSize;
    uint32_t iactorID;
    uint32_t itopicID;
    uint32_t italkingActorID;
    TESPackage::LoadGame();
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 569, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 569,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    LoadGameDataOLD(&flistentimer, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        LoadGameDataOLD(&bWaitingforLipFile, 1);
    }
    LoadNumericID(&italkingActorID, 4);
    pTalkingActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(italkingActorID));
    LoadNumericID(&iactorID, 4);
    pActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(iactorID));
    LoadNumericID(&itargetActorID__icreationTime, 4);
    pTargetActor = reinterpret_cast<MobileObject*>(static_cast<uintptr_t>(itargetActorID__icreationTime));
    LoadNumericID(&itopicID, 4);
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(itopicID);
    ptopic = dynamic_cast<TESTopic*>(FormByNumericID);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 32u) {
        LoadGameDataOLD(&apBuffer_4, 2);
        if (apBuffer_4) {
            Conversation* v10;
            Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
            if (apMemory) {
                v10 = ::new (apMemory) Conversation;
            }
            else {
                v10 = nullptr;
            }
            pConversation = v10;
            pConversation->LoadGameTES();
            LoadGameDataOLD(&v19, 2);
            if (v19 == -1) {
                pCurrentItem = nullptr;
            }
            else {
                pCurrentItem = pConversation->GetDialogueItemByIndex(v19);
            }
            LoadGameDataOLD(&apBuffer, 2);
            // Stores the sign-extended response index in the field before it resolves it.
            pCurrentResponse = reinterpret_cast<DialogueResponse*>(static_cast<intptr_t>(apBuffer));
            if (!pCurrentItem || apBuffer == -1) {
                pCurrentResponse = nullptr;
            }
            else {
                pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(apBuffer);
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
#if XFNV_64BIT
        // The save keeps 4 bytes: they fill the low half of the zeroed pointer.
        v18 = nullptr;
#endif
        LoadGameDataOLD(&v18, 4);
        pConversation = v18;
        if (v18) {
            // Old saves: the item and response indices stay in the pointer fields, sign-extended, until
            // InitLoadGame resolves them.
            LoadGameDataOLD(&v16, 2);
            pCurrentItem = reinterpret_cast<DialogueItem*>(static_cast<intptr_t>(v16));
            LoadGameDataOLD(&v17, 2);
            pCurrentResponse = reinterpret_cast<DialogueResponse*>(static_cast<intptr_t>(v17));
        }
        else {
            pCurrentItem = nullptr;
            pCurrentResponse = nullptr;
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v3 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 648,
                        pheader_2->iFormID, v3, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v6 = pheader_2->iFlags;
                int32_t v4 = pheader_2->cVersion;
                const char* v2 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 648, pheader_2->iFormID,
                    v2, v4, v6
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 648, CurrentVersion
                    );
                }
            }
            else {
                int32_t v8 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 648, v8
                );
            }
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
    char apData;
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveFormID(pTalkingActor, 0);
    apSaveGameBuffer->SaveFormID(pActor, 0);
    apSaveGameBuffer->SaveFormID(pTargetActor, 0);
    apSaveGameBuffer->SaveFormID(pTalkingActivator, 0);
    apSaveGameBuffer->SaveFormID(pTargetLocation, 0);
    apSaveGameBuffer->SaveFormID(ptopic, 0);
    apSaveGameBuffer->SaveDataEndian(&flistentimer, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bWaitingforLipFile, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bActorHeadTrack, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bCreatedPack, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bContinueMoving, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetContinueMoving, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bTargetMoveTowardStarter, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bDoNotControlTargetMovement, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotion, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iEmotionValue, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bUseIdleManager, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bSoundFile, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bReachedAmbushLocation, 1u, 0);
    apData = pConversation != nullptr;
    apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
    if (apData) {
        pConversation->SaveGameBGS(apSaveGameBuffer);
        int16_t scurrentItemIndex;
        if (pCurrentItem) {
            scurrentItemIndex = pConversation->GetDialogueItemIndex(pCurrentItem);
        }
        else {
            scurrentItemIndex = -1;
        }
        apSaveGameBuffer->SaveDataEndian(&scurrentItemIndex, 2u, 0);
        int16_t scurrentResponseIndex;
        if (pCurrentItem && pCurrentResponse) {
            scurrentResponseIndex = pCurrentItem->GetDialogueResponseIndex(pCurrentResponse);
        }
        else {
            scurrentResponseIndex = -1;
        }
        apSaveGameBuffer->SaveDataEndian(&scurrentResponseIndex, 2u, 0);
    }
}

void DialoguePackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int16_t v7;
    bool bhasConversation;
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pTalkingActor);
    apLoadGameBuffer->LoadFormID(pActor);
    apLoadGameBuffer->LoadFormID(pTargetActor);
    apLoadGameBuffer->LoadFormID(pTalkingActivator);
    apLoadGameBuffer->LoadFormID(pTargetLocation);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    ptopic = dynamic_cast<TESTopic*>(FormByNumericID);
    apLoadGameBuffer->LoadData(&flistentimer, 4u);
    apLoadGameBuffer->LoadData(&bWaitingforLipFile, 1u);
    apLoadGameBuffer->LoadData(&bActorHeadTrack, 1u);
    apLoadGameBuffer->LoadData(&bCreatedPack, 1u);
    apLoadGameBuffer->LoadData(&bContinueMoving, 1u);
    apLoadGameBuffer->LoadData(&bTargetContinueMoving, 1u);
    apLoadGameBuffer->LoadData(&bTargetMoveTowardStarter, 1u);
    apLoadGameBuffer->LoadData(&bDoNotControlTargetMovement, 1u);
    apLoadGameBuffer->LoadData(&iEmotion, 4u);
    apLoadGameBuffer->LoadData(&iEmotionValue, 4u);
    apLoadGameBuffer->LoadData(&bUseIdleManager, 1u);
    apLoadGameBuffer->LoadData(&bSoundFile, 1u);
    if (apLoadGameBuffer->GetVersion() >= 3u) {
        apLoadGameBuffer->LoadData(&bReachedAmbushLocation, 1u);
    }
    bhasConversation = false;
    apLoadGameBuffer->LoadData(&bhasConversation, 1u);
    if (bhasConversation) {
        Conversation* v4;
        int16_t v8[3];
        Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
        if (apMemory) {
            v4 = ::new (apMemory) Conversation;
        }
        else {
            v4 = nullptr;
        }
        pConversation = v4;
        pConversation->LoadGameBGS(apLoadGameBuffer);
        v8[0] = -1;
        apLoadGameBuffer->LoadData(v8, 2u);
        if (v8[0] != -1) {
            pCurrentItem = pConversation->GetDialogueItemByIndex(v8[0]);
        }
        v7 = -1;
        apLoadGameBuffer->LoadData(&v7, 2u);
        if ((pCurrentItem) && (v7 != -1)) {
            pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(v7);
        }
    }
}

void DialoguePackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v7;
    MobileObject* v8;
    MobileObject* v9;
    MobileObject* v10;
    MobileObject* v11;
    TESPackage::InitLoadGame(apLoadGameBuffer);
    if (pTalkingActor) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActor)));
        v11 = dynamic_cast<MobileObject*>(FormByNumericID);
    }
    else {
        v11 = nullptr;
    }
    pTalkingActor = v11;
    if (pActor) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
        v10 = dynamic_cast<MobileObject*>(v3);
    }
    else {
        v10 = nullptr;
    }
    pActor = v10;
    if (pTargetActor) {
        TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetActor)));
        v9 = dynamic_cast<MobileObject*>(v4);
    }
    else {
        v9 = nullptr;
    }
    pTargetActor = v9;
    if (pTalkingActivator) {
        TESForm* v5 =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActivator)));
        v8 = dynamic_cast<MobileObject*>(v5);
    }
    else {
        v8 = nullptr;
    }
    pTalkingActivator = v8;
    if (pTargetLocation) {
        TESForm* v6 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetLocation)));
        v7 = dynamic_cast<TESObjectREFR*>(v6);
    }
    else {
        v7 = nullptr;
    }
    pTargetLocation = v7;
    if (pConversation) {
        pConversation->InitItem(apLoadGameBuffer);
    }
}

void DialoguePackage::Copy(TESForm* apCopy) {
    TESPackage::Copy(apCopy);
    dynamic_cast<DialoguePackage*>(apCopy);
}

uint16_t DialoguePackage::GetSaveSize() {
    uint16_t SaveSize = TESPackage::GetSaveSize();
    uint16_t v5 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    int16_t v7 = SaveSize + 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        ++v7;
    }
    uint16_t isize = v7 + 18;
    if (pConversation) {
        isize += pConversation->GetSaveSizeTES() + 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v5,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 500,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v5, 500,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
    }
    return isize;
}

void DialoguePackage::SaveGame() {
    uint32_t iblockHeader;
    uint32_t itargetActorID;
    uint16_t iblockSize;
    uint32_t iactorID;
    uint16_t isize;
    uint32_t itopicID;
    uint32_t italkingActorID;
    TESPackage::SaveGame();
    iblockSize = 0;
    uint8_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    SaveGameDataOLD(&flistentimer, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 106u) {
        SaveGameDataOLD(&bWaitingforLipFile, 1);
    }
    italkingActorID = 0;
    if (pTalkingActor) {
        italkingActorID = pTalkingActor->iFormID;
    }
    SaveNumericID(&italkingActorID, 4);
    iactorID = 0;
    if (pActor) {
        iactorID = pActor->iFormID;
    }
    SaveNumericID(&iactorID, 4);
    itargetActorID = 0;
    if (pTargetActor) {
        itargetActorID = pTargetActor->iFormID;
    }
    SaveNumericID(&itargetActorID, 4);
    itopicID = 0;
    if (ptopic) {
        itopicID = ptopic->iFormID;
    }
    SaveNumericID(&itopicID, 4);
    isize = 0;
    if (pConversation) {
        isize = pConversation->GetSaveSizeTES();
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&isize, 2u);
    if (isize) {
        int16_t icurrentItemIndex;
        int16_t icurrentResponseIndex;
        pConversation->SaveGameTES();
        icurrentItemIndex = -1;
        if (pCurrentItem) {
            icurrentItemIndex = pConversation->GetDialogueItemIndex(pCurrentItem);
        }
        SaveGameDataOLD(&icurrentItemIndex, 2);
        icurrentResponseIndex = -1;
        if ((pCurrentItem) && (pCurrentResponse)) {
            icurrentResponseIndex = pCurrentItem->GetDialogueResponseIndex(pCurrentResponse);
        }

        SaveGameDataOLD(&icurrentResponseIndex, 2);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 561,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 561,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pblockEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = pblockSize;
        if (pblockEnd > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\DialoguePackage.cpp", 561
            );
        }
        // The save buffer is raw bytes: the 2-byte size placeholder is patched in place.
        *reinterpret_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pblockEnd - pblockStart);
    }
}

void DialoguePackage::InitLoadGame() {
    TESPackage::InitLoadGame();
    TESForm* FormByNumericID =
        TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTalkingActor)));
    pTalkingActor = dynamic_cast<Actor*>(FormByNumericID);
    TESForm* v2 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pActor)));
    pActor = dynamic_cast<Actor*>(v2);
    TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetActor)));
    pTargetActor = dynamic_cast<Actor*>(v3);
    if ((pSaveLoadGameOLD->GetCurrentVersion() >= 32u) && (pConversation)) {
        pConversation->InitLoadGameTES();
    }

    if (pSaveLoadGameOLD->GetCurrentVersion() < 32u) {
        // Old saves: LoadGame left the item and response indices in the pointer fields; their low 16 bits are read.
        Conversation* pOldConversation = pConversation;
        int16_t icurrentItemIndex = static_cast<int16_t>(reinterpret_cast<uintptr_t>(pCurrentItem));
        int16_t icurrentResponseIndex = static_cast<int16_t>(reinterpret_cast<uintptr_t>(pCurrentResponse));
        if (pOldConversation) {
            Conversation* v4;
            Conversation* apMemory = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
            if (apMemory) {
                v4 = ::new (apMemory) Conversation(pActor, pTargetActor, ptopic);
            }
            else {
                v4 = nullptr;
            }
            pConversation = v4;
            if (icurrentItemIndex == -1) {
                pCurrentItem = nullptr;
            }
            else {
                pCurrentItem = pConversation->GetDialogueItemByIndex(icurrentItemIndex);
            }
            if (!pCurrentItem || icurrentResponseIndex == -1) {
                pCurrentResponse = nullptr;
            }
            else {
                pCurrentResponse = pCurrentItem->GetDialogueResponseByIndex(icurrentResponseIndex);
            }
            pConversation->SetCurrentItem(pCurrentItem);
            pCurrentItem->SetCurrentResponse(pCurrentResponse);
        }
    }
}

bool DialoguePackage::IsPackageOwner(MobileObject* apOwner) {
    return (bool)((uint8_t)(static_cast<const void*>(apOwner) == static_cast<const void*>(pActor)));
}

void DialoguePackage::DeleteSoundHandle() {
    if (pTalkingActor) {
        pTalkingActor->StopCurrentDialogue();
    }
}

bool DialoguePackage::GetContinueMoving() {
    return (bool)((uint8_t)bContinueMoving);
}

void DialoguePackage::SetContinueMoving(bool abContinueMoving) {
    bContinueMoving = (bool)((uint8_t)abContinueMoving);
}

void DialoguePackage::SetTargetContinueMoving(bool abMove) {
    bTargetContinueMoving = (bool)((uint8_t)abMove);
}

void DialoguePackage::SetTargetMoveTowardStarter(bool abMove) {
    bTargetMoveTowardStarter = (bool)((uint8_t)abMove);
}

bool DialoguePackage::IsCreated() {
    return (bool)((uint8_t)bCreatedPack);
}

bool DialoguePackage::GetTargetContinueMoving() {
    return (bool)((uint8_t)bTargetContinueMoving);
}

bool DialoguePackage::GetTargetMoveTowardStarter() {
    return (bool)((uint8_t)bTargetMoveTowardStarter);
}

void DialoguePackage::SetReachedAmbushLocation(bool abFlag) {
    bReachedAmbushLocation = (bool)((uint8_t)abFlag);
}

bool DialoguePackage::GetReachedAmbushLocation() {
    return (bool)((uint8_t)bReachedAmbushLocation);
}

bool DialoguePackage::GetShouldHeadTrackTarget() {
    return (bool)((uint8_t)bActorHeadTrack);
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
    bool bcheckforIdle = true;
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
    TESObjectREFR* pspeaker = pCurrentItem->pSpeaker;
    if (pspeaker->IsActor()) {
        Actor* pact = static_cast<Actor*>(pCurrentItem->pSpeaker);
        if (pact && abUseSound) {
            BSStringT<char> audioFilePath;
            if (pTalkingActor) {
                pTalkingActor->StopCurrentDialogue();
                pTalkingActor = nullptr;
            }
            bool bnosound = true;
            if (pCurrentResponse->pVoiceSound && bSoundFile) {
                uint32_t isoundFileID = pCurrentResponse->pVoiceSound->iFormID;
                XFNV_INLINED_LOCAL(BSSoundHandle, soundHandle, std::nullopt, std::nullopt);
                pact->PlaySound(
                    &soundHandle, isoundFileID, false,
                    BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN | BSGameSound::SOUND_FLAG_ONE_SHOT |
                        BSGameSound::SOUND_FLAG_IS_3D,
                    true
                );
                BSAudioManager* mgr = &BSAudioManager::QInstance();
                mgr->SetCompletionCallback(
                    soundHandle.iSoundID, &MobileObject::SoundFileCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                );
                bSoundFile = false;
                bnosound = false;
            }
            if (bnosound && bSoundFile) {
                audioFilePath.Set(pCurrentResponse->cVoice);
                if (!pact->pCurrentProcess->GetLipAnim() && !pact->pCurrentProcess->GetWaitingforLipFile() &&
                    bBackgroundLoadLipFiles.Bool() && !pact->pCurrentProcess->GetLipFileFailed()) {
                    iEmotion = pCurrentResponse->eEmotion;
                    iEmotionValue = pCurrentResponse->uiEmotion;
                    if (pCurrentItem) {
                        pCurrentItem->RunResult(TIRS_BEGIN);
                    }
                    pTalkingActor = pact;
                    pact->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                    pact->SpeakSoundFunction(
                        audioFilePath.String(), handle, pCurrentResponse->eEmotion, pCurrentResponse->uiEmotion,
                        pCurrentResponse->cText.GetLength(), pCurrentResponse->pSpeakerIdle,
                        pCurrentResponse->pListenIdle, pTargetActor, true, false, true, true, bUseIdleManager
                    );
                    if (pTalkingActivator &&
                        static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference())->pTempRef == pact) {
                        MobileObject* ptarg = pTargetActor;
                        if (pTargetActor == pTalkingActivator) {
                            ptarg = pActor;
                        }
                        pTalkingActor = pTalkingActivator;
                        pTalkingActivator->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                        pTalkingActivator->SpeakSoundFunction(
                            audioFilePath.String(), handle, pCurrentResponse->eEmotion, pCurrentResponse->uiEmotion,
                            pCurrentResponse->cText.GetLength(), pCurrentResponse->pSpeakerIdle,
                            pCurrentResponse->pListenIdle, ptarg, false, false, false, true, true
                        );
                    }
                    if (pact->pCurrentProcess->GetWaitingforLipFile()) {
                        bWaitingforLipFile = true;
                    }
                    else {
                        BSAudioManager* mgr = &BSAudioManager::QInstance();
                        mgr->SetCompletionCallback(
                            handle.iSoundID, &MobileObject::DialogueConversationCallBack,
                            reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                        );
                        if (pActor != pTargetActor) {
                            flistentimer = 50.0f;
                        }
                        pact->bVoiceFileDone = false;
                    }
                    return;
                }
                if (pact->pCurrentProcess->GetWaitingforLipFile() && !pact->pCurrentProcess->GetLipFileFailed()) {
                    return;
                }
                if (!bBackgroundLoadLipFiles.Bool() && pCurrentItem) {
                    pCurrentItem->RunResult(TIRS_BEGIN);
                }
                pTalkingActor = pact;
                pact->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                pact->SpeakSoundFunction(
                    audioFilePath.String(), handle, pCurrentResponse->eEmotion, pCurrentResponse->uiEmotion,
                    pCurrentResponse->cText.GetLength(), pCurrentResponse->pSpeakerIdle, pCurrentResponse->pListenIdle,
                    pTargetActor, true, false, false, true, bUseIdleManager
                );
                if (pTalkingActivator &&
                    static_cast<BGSTalkingActivator*>(pTalkingActivator->GetObjectReference())->pTempRef == pact) {
                    MobileObject* ptarg = pTargetActor;
                    if (pTargetActor == pTalkingActivator) {
                        ptarg = pActor;
                    }
                    pTalkingActor = pTalkingActivator;
                    pTalkingActivator->SetShouldUseEmotionAnimation(pCurrentResponse->bUseEmotion);
                    pTalkingActivator->SpeakSoundFunction(
                        audioFilePath.String(), handle, pCurrentResponse->eEmotion, pCurrentResponse->uiEmotion,
                        pCurrentResponse->cText.GetLength(), pCurrentResponse->pSpeakerIdle,
                        pCurrentResponse->pListenIdle, ptarg, false, false, false, true, true
                    );
                }
                BSAudioManager* mgr = &BSAudioManager::QInstance();
                mgr->SetCompletionCallback(
                    handle.iSoundID, &MobileObject::DialogueConversationCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pActor->iFormID))
                );
                pact->bVoiceFileDone = false;
                bcheckforIdle = false;
                pTalkingActor = pact;
                pact->pCurrentProcess->SetLipAnim(nullptr);
                bWaitingforLipFile = false;
                pact->pCurrentProcess->SetLipFileFailed(false);
                if (pActor != pTargetActor) {
                    flistentimer = 2.0f;
                }
                // Asks both actors for their animation and deliberately keeps neither result, as the game does.
                pTalkingActor->GetAnimation();
                pTargetActor->GetAnimation();
            }
        }
        if (bcheckforIdle) {
            if (pActor->pCurrentProcess) {
                pActor->pCurrentProcess->SetupSpecialIdle(pActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, false, true, true);
            }
            // Tests the talking actor's process and then idles the target's, as the game does.
            if (pActor != pTalkingActor && pTalkingActor && pTalkingActor->pCurrentProcess) {
                pTargetActor->pCurrentProcess->SetupSpecialIdle(
                    pTargetActor, nullptr, AnimIdle::PLAY_TYPE_AUTO, false, true, true
                );
            }
        }
        return;
    }
    if (pspeaker && abUseSound) {
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
                    audioFilePath.String(), handle, pCurrentResponse->eEmotion, pCurrentResponse->uiEmotion,
                    pCurrentResponse->cText.GetLength(), pCurrentResponse->pSpeakerIdle, pCurrentResponse->pListenIdle,
                    pTargetActor, false, false, false, true, true
                );
                BSAudioManager* mgr = &BSAudioManager::QInstance();
                mgr->SetCompletionCallback(
                    handle.iSoundID, &MobileObject::DialogueConversationCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(pTalkingActivator->iFormID))
                );
                pTalkingActivator->bVoiceFileDone = false;
            }
            else {
                uint32_t audioFlags =
                    BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_VOICE | BSGameSound::SOUND_FLAG_IS_3D;
                XFNV_INLINED_LOCAL(BSSoundHandle, soundHandle, std::nullopt, std::nullopt);
                handle = *BSAudio::QInstance().GetSoundHandleByFilename(&soundHandle, acAudioFile, audioFlags, nullptr);
                NiPoint3 kSpeakerLoc = pspeaker->GetLocationOnReference();
                // Reads the maximum distance first.
                float fMaxDist = BSAudio::GetMaxDialogDist();
                float fMinDist = BSAudio::GetMinDialogDist();
                handle.SetMinMax(fMinDist, fMaxDist);
                handle.SetPosition(kSpeakerLoc.x, kSpeakerLoc.y, kSpeakerLoc.z);
                handle.SetObjectToFollow(pspeaker->Get3D());
                handle.PlayAfter(1, 0);
            }
        }
        if ((this == pPlayer->pClosestConversation || ReferenceProcessLists.bShowSubtitle) &&
            (pConversation->GetCurrentItem() && pConversation->GetCurrentItem()->GetCurrentResponse() &&
             pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String()) &&
            (gs_bGeneralSubtitles_Gameplay.Bool() ||
             (pspeaker->IsActor() && static_cast<uint8_t>(static_cast<Actor*>(pspeaker)->GetShowSubtitles()))))

        // GetShowSubtitles: only the low byte of the result is tested.
        {
            // Copies the location, then the handle with operator=, then reads the text.
            NiPoint3 kSpeakerLoc = pspeaker->GetLocationOnReference();
            XFNV_INLINED_LOCAL(BSSoundHandle, subtitleHandle, std::nullopt, std::nullopt);
            subtitleHandle = handle;
            const char* pText = pConversation->GetCurrentItem()->GetCurrentResponse()->cText.String();
            Interface::ShowSubtitle(pText, subtitleHandle, kSpeakerLoc, pTargetActor, false);
        }

        if (flistentimer <= 0.0 && pTargetActor != pspeaker) {
            flistentimer = 10.0f;
        }
    }
}
