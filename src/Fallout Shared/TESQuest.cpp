#include "Fallout Shared/TESQuest.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bActivateAllQuestScripts("bActivateAllQuestScripts:General", false);

QUEST_DATA* QUEST_DATA::Init() {
    ucFlags = 0;
    ucPriority = 0;
    fQuestDelayTime = 0.0f;
    return this;
}

void QUEST_DATA::Endian() {
    BSEndian(fQuestDelayTime, false);
}

TESQuest::TESQuest() : pScriptLocals(nullptr) {
    data.Init();
    SetFormType(QUST_ID);
    TESQuest::InitializeData();
}

TESQuest::~TESQuest() {
    TESQuest::ClearData();
}

void TESQuest::InitializeData() {
    SetEnabled(1);
    cCurrentStage = 0;
}

void TESQuest::ClearData() {
    GetConditions()->ClearAllConditionItems();
    ClearStageList();
    ClearObjectiveList();
    if (pScriptLocals) {
        ScriptLocals* pLocals = pScriptLocals;
        if (pLocals) {
            delete pLocals;
        }
        pScriptLocals = nullptr;
    }
}

// When the EDID chunk is empty (malformed data), its buffer is initialised to "" (one zeroed byte, allocated only for
// an empty chunk). QUST. The stage item's own chunks (SCHR, SCDA, SCRO, SCVR, SLSD) are read by
// TESQuestStageItem::Load, so those cases go back to the chunk read instead of calling NextChunk.
bool TESQuest::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != QUST_ID) {
        return false;
    }
    TESQuestStage* pStage = nullptr;
    TESQuestStageItem* pStageItem = nullptr;
    TESQuestTarget* pTarget = nullptr;
    BGSQuestObjective* pObjective = nullptr;
    uint8_t ucLogIndex = 0;
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        bool bChunkRead = false;
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten, so it is cleared first (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case DATA_ID:
                // The DATA chunk is a raw 8-byte buffer.
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
                break;
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                // The chunk holds the script's form ID, which is stored unresolved.
                TESScriptableForm::SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                TESScriptableForm::InitItem(this);
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case QOBJ_ID: {
                pObjective = new BGSQuestObjective;
                pObjective->Load(apFile);
                GetObjectiveList()->AddTail(pObjective);
                pObjective->SetOwner(this);
                break;
            }
            case QSTA_ID: {
                pStage = nullptr;
                pStageItem = nullptr;
                pTarget = new TESQuestTarget;
                pTarget->Load(apFile);
                if (!pObjective) {
                    pObjective = new BGSQuestObjective;
                    GetObjectiveList()->AddTail(pObjective);
                    pObjective->SetOwner(this);
                }
                pObjective->GetTargetList()->AddTail(pTarget);
                break;
            }
            case INDX_ID: {
                pStage = new TESQuestStage;
                pStage->Load(apFile);
                GetStageList()->AddTail(pStage);
                break;
            }
            case QSDT_ID:
                if (pStage) {
                    pStageItem = new TESQuestStageItem;
                    pStageItem->Load(apFile);
                    uint8_t ucIndex = ucLogIndex;
                    pStageItem->SetLogEntryID(ucIndex);
                    ucLogIndex = static_cast<uint8_t>(ucLogIndex + 1);
                    pStage->GetStageItemList()->AddTail(pStageItem);
                }
                bChunkRead = true;
                break;
            case CNAM_ID:
                if (pStageItem) {
                    pStageItem->SetHasLogEntry(true);
                }
                break;
            case SCDA_ID:
            case SCHR_ID:
            case SCRO_ID:
            case SCVR_ID:
            case SLSD_ID:
                if (pStageItem) {
                    pStageItem->Load(apFile);
                    bChunkRead = true;
                }
                break;
            case CIDA_ID:
                if (pStageItem) {
                    pStageItem->GetConditionList()->Load(apFile);
                }
                else if (pTarget) {
                    pTarget->GetConditionList()->Load(apFile);
                }
                else {
                    GetConditions()->Load(apFile);
                }
                break;
            default: break;
        }
        if (bChunkRead) {
            continue;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    if (apFile->QEndian()) {
        data.Endian();
    }
    return true;
}

void TESQuest::Save() {
    StartForm();
    TESScriptableForm::Save();
    TESFullName::Save();
    TESTexture::Save(ICON_ID);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    objConditions.Save();
    for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {}
    for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
         pObjectiveList && !pObjectiveList->IsEmpty();
         pObjectiveList = pObjectiveList->m_pkNext) {
        pObjectiveList->m_item->Save();
    }
    CloseForm();
}

uint16_t TESQuest::GetSaveSize(uint32_t aiFlags) {
    uint16_t usSize = TESForm::GetSaveSize(aiFlags);
    uint16_t usBaseSize = usSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSize += 6;
    }
    if ((aiFlags & CHANGE_QUEST_FLAGS) != 0) {
        ++usSize;
    }
    if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
        BSSimpleList<TESQuestStage*>* pStages = &m_listStages;
        ++usSize;
        while (pStages) {
            TESQuestStage* pStage = pStages->m_item;
            if (pStage) {
                usSize += 2 + 5 * pStage->listStageItems.ItemsInList() + 1;
            }
            pStages = pStages->m_pkNext;
        }
    }
    if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
        usSize += pScriptLocals->GetSaveSize();
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
                3920,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBaseSize,
                3920,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp"
            );
        }
    }
    return usSize;
}

void TESQuest::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    uint16_t usBlockSize = 0;
    uint16_t* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    if ((aiFlags & CHANGE_QUEST_FLAGS) != 0) {
        uint8_t ucFlags = data.ucFlags;
        SaveGameDataOLD(&ucFlags, sizeof(ucFlags));
    }
    if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
        uint8_t ucStageCount = 0;
        uint8_t* pStageCount = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveGameDataOLD(&ucStageCount, sizeof(ucStageCount));
        for (TESQuestStage* pStage : m_listStages) {
            if (pStage) {
                uint8_t ucStageIndex = pStage->m_Data.ucIndex;
                uint8_t ucDone = pStage->m_Data.bIsDone;
                SaveGameDataOLD(&ucStageIndex, sizeof(ucStageIndex));
                SaveGameDataOLD(&ucDone, sizeof(ucDone));
                uint8_t ucItemCount = 0;
                uint8_t* pItemCount = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
                SaveGameDataOLD(&ucItemCount, sizeof(ucItemCount));
                for (TESQuestStageItem* pStageItem : pStage->listStageItems) {
                    if (pStageItem) {
                        uint8_t ucItemIndex = pStageItem->ucIndex;
                        Date* pDate = pStageItem->m_pLogDate;
                        SaveGameDataOLD(&ucItemIndex, sizeof(ucItemIndex));
                        if (pDate) {
                            pSaveLoadGameOLD->SaveGameDataOLD(pDate, sizeof(*pDate));
                        }
                        else {
                            Date kDate;
                            pSaveLoadGameOLD->SaveGameDataOLD(&kDate, sizeof(kDate));
                        }
                        ++ucItemCount;
                    }
                }
                *pItemCount = ucItemCount;
                ++ucStageCount;
            }
        }
        *pStageCount = ucStageCount;
    }
    if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0) {
        pScriptLocals->SaveGame();
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
                4003,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                4003,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size: the buffer's advance past the size word, compared as unsigned addresses.
        char* pBufferNow = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        char* pBlockStart = static_cast<char*>(static_cast<void*>(pBlockSize));
        if (pBufferNow > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                4003
            );
        }
        *pBlockSize = static_cast<uint16_t>(pBufferNow - pBlockStart);
    }
}

void TESQuest::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_FLAGS)) {
        apSaveGameBuffer->SaveDataEndian(&data, 1u, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT_DELAY)) {
        apSaveGameBuffer->SaveDataEndian(&data.fQuestDelayTime, sizeof(data.fQuestDelayTime), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
        uint32_t uiStageCount = 0;
        uint32_t uiStageCountPos = apSaveGameBuffer->StartVariableSizedValue();
        for (TESQuestStage* pStage : m_listStages) {
            if (pStage) {
                uint8_t ucStageIndex = pStage->m_Data.ucIndex;
                uint8_t ucDone = pStage->m_Data.bIsDone;
                apSaveGameBuffer->SaveDataEndian(&ucStageIndex, sizeof(ucStageIndex), 0);
                apSaveGameBuffer->SaveDataEndian(&ucDone, sizeof(ucDone), 0);
                uint32_t uiItemCount = 0;
                uint32_t uiItemCountPos = apSaveGameBuffer->StartVariableSizedValue();
                for (BSSimpleList<TESQuestStageItem*>* pList = &pStage->listStageItems; pList;
                     pList = pList->m_pkNext) {
                    TESQuestStageItem* pStageItem = pList->m_item;
                    if (pStageItem) {
                        uint8_t ucItemIndex = pStageItem->ucIndex;
                        Date* pDate = pStageItem->m_pLogDate;
                        bool bHasDate = pDate != nullptr;
                        apSaveGameBuffer->SaveDataEndian(&ucItemIndex, sizeof(ucItemIndex), 0);
                        apSaveGameBuffer->SaveDataEndian(&bHasDate, sizeof(bHasDate), 0);
                        if (bHasDate) {
                            apSaveGameBuffer->SaveDataEndian(pDate, sizeof(*pDate), 0);
                        }
                        ++uiItemCount;
                    }
                }
                apSaveGameBuffer->SaveVariableSizedValue(uiItemCount, uiItemCountPos);
                ++uiStageCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(uiStageCount, uiStageCountPos);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT)) {
        pScriptLocals->SaveGame(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
        uint32_t uiObjectiveCount = 0;
        uint32_t uiObjectiveCountPos = apSaveGameBuffer->StartVariableSizedValue();
        for (BGSQuestObjective* pObjective : m_listObjectives) {
            if (pObjective) {
                QUEST_OBJECTIVE_STATE eState = pObjective->eState;
                if (eState) {
                    uint32_t uiObjectiveIndex = pObjective->iIndex;
                    apSaveGameBuffer->SaveDataEndian(&uiObjectiveIndex, sizeof(uiObjectiveIndex), 0);
                    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
                    ++uiObjectiveCount;
                }
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(uiObjectiveCount, uiObjectiveCountPos);
    }
}

// The quest's old-format load: the form's data, the block header, then by the change flags the DATA flags, the stages'
// done flags and the log dates of their items, and the script's locals; the block size is checked last.
void TESQuest::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    uint16_t sBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iBlockID;
        pSaveLoadGameOLD->LoadGameDataOLD(&iBlockID, sizeof(iBlockID));
        if (iBlockID != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                    4015,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                    4015,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    // The saved bytes are read into char locals (the reader's buffer type) and used zero-extended.
    if (aiFlags & CHANGE_QUEST_FLAGS) {
        char cFlags;
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cFlags), sizeof(cFlags));
        GetData()->ucFlags = static_cast<uint8_t>(cFlags);
    }
    if (aiFlags & CHANGE_QUEST_STAGES) {
        char cStageCount;
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cStageCount), sizeof(cStageCount));
        for (int32_t i = 0; i < static_cast<uint8_t>(cStageCount); ++i) {
            char cStageIndex;
            TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cStageIndex), sizeof(cStageIndex));
            char cDone;
            TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cDone), sizeof(cDone));
            TESQuestStage* pStage = GetStage(static_cast<uint8_t>(cStageIndex));
            if (pStage) {
                pStage->SetDone(static_cast<uint8_t>(cDone));
            }
            else {
                Warning(
                    "SAVELOAD: Could not find stage %i in quest %s during load.",
                    static_cast<uint32_t>(static_cast<uint8_t>(cStageIndex)),
                    GetEditorName()
                );
            }
            char cItemCount;
            TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cItemCount), sizeof(cItemCount));
            for (int32_t j = 0; j < static_cast<uint8_t>(cItemCount); ++j) {
                Date kLogDate;
                char cItemIndex;
                TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cItemIndex), sizeof(cItemIndex));
                pSaveLoadGameOLD->LoadGameDataOLD(&kLogDate, sizeof(kLogDate));
                if (pStage) {
                    TESQuestStageItem* pItem = pStage->GetStageItem(static_cast<uint8_t>(cItemIndex));
                    if (pItem) {
                        if (kLogDate.GetDate() || kLogDate.GetYear()) {
                            Date* pLogDate = pItem->GetLogDate();
                            if (pLogDate) {
                                *pLogDate = kLogDate;
                            }
                            else {
                                Date* pNewDate = new Date(kLogDate);
                                pItem->SetLogDate(pNewDate);
                            }
                        }
                        else {
                            Date* pLogDate = pItem->GetLogDate();
                            if (pLogDate) {
                                delete pLogDate;
                                pItem->SetLogDate(nullptr);
                            }
                        }
                    }
                }
            }
        }
        cCurrentStage = 0;
        FindCurrentStage();
    }
    if (aiFlags & CHANGE_QUEST_SCRIPT) {
        if (!pScriptLocals) {
            // No locals to load into: the saved ones are read into a scratch set and dropped.
            ScriptLocals* pScratch = new ScriptLocals;
            pScratch->LoadGame();
            if (pScratch) {
                delete pScratch;
            }
            pSaveLoadGameOLD->QueueRemoveChange(CHANGE_QUEST_SCRIPT);
        }
        else {
            pScriptLocals->LoadGame();
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pBlockEnd > pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                    4112,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else if (pBlockEnd < pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockStart + sBlockSize - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                    4112,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pBlockEnd > pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                4112,
                cVersion
            );
        }
        else if (pBlockEnd < pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockStart + sBlockSize - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESQuest.cpp",
                4112,
                cVersion
            );
        }
    }
}

void TESQuest::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_FLAGS)) {
        apLoadGameBuffer->LoadData(&data, 1u);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT_DELAY)) {
        apLoadGameBuffer->LoadData(&data.fQuestDelayTime, sizeof(data.fQuestDelayTime));
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
        uint32_t uiStageCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t uiLoop = 0; uiLoop < uiStageCount; ++uiLoop) {
            uint8_t ucStageIndex = 0;
            uint8_t ucDone = 0;
            apLoadGameBuffer->LoadData(&ucStageIndex, sizeof(ucStageIndex));
            apLoadGameBuffer->LoadData(&ucDone, sizeof(ucDone));
            TESQuestStage* pStage = GetStage(ucStageIndex);
            if (pStage) {
                pStage->SetDone(ucDone);
            }
            else {
                const char* pEditorName = GetEditorName();
                SaveGameWarning(
                    "SAVELOAD: Could not find stage %i in quest %s during load.",
                    ucStageIndex,
                    pEditorName
                );
            }
            uint32_t uiItemCount = apLoadGameBuffer->LoadVariableSizedValue();
            for (uint32_t uiItemLoop = 0; uiItemLoop < uiItemCount; ++uiItemLoop) {
                uint8_t ucItemIndex = 0;
                bool bHasDate = false;
                Date kDate;
                apLoadGameBuffer->LoadData(&ucItemIndex, sizeof(ucItemIndex));
                apLoadGameBuffer->LoadData(&bHasDate, sizeof(bHasDate));
                if (bHasDate) {
                    apLoadGameBuffer->LoadData(&kDate, sizeof(kDate));
                }
                if (bHasDate && pStage) {
                    TESQuestStageItem* pStageItem = pStage->GetStageItem(ucItemIndex);
                    if (pStageItem) {
                        Date* pDate = new Date(kDate);
                        pStageItem->SetLogDate(pDate);
                    }
                }
            }
        }
        cCurrentStage = 0;
        FindCurrentStage();
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT)) {
        if (pScriptLocals) {
            pScriptLocals->LoadGame(apLoadGameBuffer);
        }
        else {
            ScriptLocals kTempLocals;
            kTempLocals.LoadGame(apLoadGameBuffer);
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
        uint32_t uiObjectiveCount = apLoadGameBuffer->LoadVariableSizedValue();
        for (uint32_t uiObjectiveLoop = 0; uiObjectiveLoop < uiObjectiveCount; ++uiObjectiveLoop) {
            uint32_t uiObjectiveIndex = 0;
            QUEST_OBJECTIVE_STATE eState = QOS_DORMANT;
            apLoadGameBuffer->LoadData(&uiObjectiveIndex, sizeof(uiObjectiveIndex));
            apLoadGameBuffer->LoadData(&eState, sizeof(eState));
            BGSQuestObjective* pObjective = GetObjective(uiObjectiveIndex);
            if (pObjective) {
                pObjective->eState = eState;
            }
        }
    }
}

void TESQuest::Revert(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_QUEST_STAGES) != 0) {
        for (TESQuestStage* pStage : m_listStages) {
            if (pStage) {
                pStage->SetDone(0);
                for (TESQuestStageItem* pStageItem : pStage->listStageItems) {
                    if (pStageItem) {
                        if (Date* pLogDate = pStageItem->m_pLogDate) {
                            pStageItem->SetLogDate(nullptr);
                            delete pLogDate;
                        }
                    }
                }
            }
        }
        cCurrentStage = 0;
    }
    if ((aiFlags & CHANGE_QUEST_SCRIPT) != 0 && pScriptLocals) {
        pScriptLocals->Revert_0();
    }
}

void TESQuest::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_STAGES)) {
        for (TESQuestStage* pStage : m_listStages) {
            if (pStage) {
                pStage->SetDone(0);
                for (BSSimpleList<TESQuestStageItem*>* pList = &pStage->listStageItems; pList;
                     pList = pList->m_pkNext) {
                    TESQuestStageItem* pStageItem = pList->m_item;
                    if (pStageItem && pStageItem->m_pLogDate) {
                        Date* pDate = pStageItem->m_pLogDate;
                        if (pDate) {
                            delete pDate;
                        }
                        pStageItem->m_pLogDate = nullptr;
                    }
                }
            }
        }
        cCurrentStage = 0;
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_SCRIPT) && pScriptLocals) {
        pScriptLocals->Revert(apLoadGameBuffer);
    }

    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_QUEST_OBJECTIVES)) {
        for (BGSQuestObjective* pObjective : m_listObjectives) {
            if (pObjective) {
                pObjective->eState = QOS_DORMANT;
            }
        }
    }
}

void TESQuest::CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    BGSChangeFlags kChangeFlags;
    kChangeFlags = apSaveGameBuffer->GetChangeFlags();
    if (kChangeFlags.CheckFlags(CHANGE_QUEST_SCRIPT) && !pScriptLocals) {
        apSaveGameBuffer->RemoveChange(CHANGE_QUEST_SCRIPT);
    }
}

void TESQuest::InitItem() {
    if (!GetInitialized()) {
        TESScriptableForm::InitItem(this);
        if (pFormScript) {
            pScriptLocals = pFormScript->CloneLocals();
            pFormScript->pOwnerQuest = this;
            if (pScriptLocals) {
                ForceChange(CHANGE_QUEST_SCRIPT);
            }
            if (bActivateAllQuestScripts.Bool()) {
                SetEnabled(1);
            }
        }
        objConditions.InitItem(this);
        for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
            pList->m_item->InitItem(this);
        }
        for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
             pObjectiveList && !pObjectiveList->IsEmpty();
             pObjectiveList = pObjectiveList->m_pkNext) {
            pObjectiveList->m_item->InitItem(this);
        }
        SetStartsEnabled(GetEnabled());
        SetInitialized(true);
    }
}

// The editor ID is copied to an _alloca buffer, compared with the current one and stored when it changed.
bool TESQuest::SetFormEditorID(const char* apEditorID) {
    uint32_t uiSize = BSstrlen(apEditorID) + 1;
    // A 16-byte aligned stack buffer of the string's size.
    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
    BSstrcpy(pBuffer, uiSize, apEditorID);
    if (!strEditorID.StrCmp(pBuffer, true)) {
        return false;
    }
    strEditorID.Set(pBuffer, 0);
    return true;
}

const char* TESQuest::GetEditorName() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }

    return "";
}

void TESQuest::SetAllObjectivesComplete(int32_t aiState) {
    for (BSSimpleList<BGSQuestObjective*>* pList = GetObjectiveList(); pList && pList->m_item;
         pList = pList->m_pkNext) {
        QUEST_OBJECTIVE_STATE eState = static_cast<QUEST_OBJECTIVE_STATE>(pList->m_item->GetDisplayed() + 2);
        pList->m_item->SetState(eState);
    }
}

BGSQuestObjective* TESQuest::GetObjective(uint32_t aiIndex) {
    for (BSSimpleList<BGSQuestObjective*>* pNode = GetObjectiveList(); pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        BGSQuestObjective* pObjective = pNode->GetItem();
        if (pObjective && pObjective->GetIndex() == aiIndex) {
            return pObjective;
        }
    }
    return nullptr;
}

bool TESQuest::SetStageDone(uint8_t aucIndex) {
    BSSimpleList<TESQuestStage*>* pList;
    for (pList = &m_listStages;; pList = pList->m_pkNext) {
        if (!pList || pList->IsEmpty()) {
            return false;
        }
        if (pList->m_item->m_Data.ucIndex == aucIndex) {
            break;
        }
    }
    if (!GetCompleted() || GetAllowRepeatStages()) {
        SetEnabled(1);
    }
    if (aucIndex > static_cast<int32_t>(cCurrentStage)) {
        cCurrentStage = aucIndex;
    }
    pList->m_item->SetDone(this, false);
    AddChange(CHANGE_QUEST_STAGES);
    return true;
}

void TESQuest::SetEnabled(uint8_t abEnabled) {
    if (abEnabled) {
        GetData()->ucFlags |= QUEST_DATA::FLAG_ENABLED;
    }
    else {
        GetData()->ucFlags &= ~QUEST_DATA::FLAG_ENABLED;
    }
    AddChange(CHANGE_QUEST_FLAGS);
}

TESCondition* TESQuest::GetConditions() {
    return &objConditions;
}

bool TESQuest::GetEnabled() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_ENABLED) != 0;
}

void TESQuest::RunScript() {
    if (TESScriptableForm::GetFormScript()) {
        TESScriptableForm::GetFormScript()->Run(nullptr, pScriptLocals, nullptr, false);
    }
}

TESQuestStage* TESQuest::GetStage(uint8_t acIndex) {
    for (BSSimpleList<TESQuestStage*>* pNode = GetStageList(); pNode; pNode = pNode->GetNext()) {
        TESQuestStage* pStage = pNode->GetItem();
        if (pStage && pStage->GetIndex() == acIndex) {
            return pStage;
        }
    }
    return nullptr;
}

uint8_t TESQuest::GetStageIndexForStageItem(TESQuestStageItem* apStageItem) {
    for (BSSimpleList<TESQuestStage*>* pStages = &m_listStages; pStages && !pStages->IsEmpty();
         pStages = pStages->m_pkNext) {
        TESQuestStage* pStage = pStages->m_item;
        if (apStageItem == pStage->GetStageItem(apStageItem->ucIndex)) {
            return pStage->m_Data.ucIndex;
        }
    }
    return 0;
}

bool TESQuest::ValidateTargetList(
    BSSimpleList<TESQuestTarget*>* apListToValidate,
    BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList
) {
    BSSimpleList<TESQuestTarget*>* pSuppliedList = apListToValidate;
    BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
    if (apListToValidate && pObjectiveList) {
        while (pObjectiveList) {
            if (pObjectiveList->m_item &&
                pObjectiveList->m_item->eState == QOS_DISPLAYED &&
                apPlayerObjectiveList->IsInList(pObjectiveList->m_item)) {
                for (TESQuestTarget* pTarget : pObjectiveList->m_item->ListTargets) {
                    if (pTarget) {
                        TESObjectREFR* pReference = pTarget->GetReference(false);
                        if (pTarget->objConditions.IsTrue(pReference, nullptr)) {
                            if (!pSuppliedList || !pSuppliedList->m_item || pSuppliedList->m_item != pTarget) {
                                return false;
                            }
                            pSuppliedList = pSuppliedList->m_pkNext;
                        }
                    }
                }
            }
            pObjectiveList = pObjectiveList->m_pkNext;
        }
    }
    return !pSuppliedList || pSuppliedList->IsEmpty();
}

void TESQuest::UpdateCurrentTargetList(
    BSSimpleList<TESQuestTarget*>* apListToFill,
    BSSimpleList<BGSQuestObjective*>* apPlayerObjectiveList
) {
    if (apListToFill) {
        apListToFill->RemoveAll();
        if (!GetCompleted()) {
            for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = GetObjectiveList();
                 pObjectiveList && pObjectiveList->m_item;
                 pObjectiveList = pObjectiveList->m_pkNext) {
                if (apPlayerObjectiveList->IsInList(pObjectiveList->m_item)) {
                    pObjectiveList->m_item->UpdateCurrentTargetList(apListToFill);
                }
            }
        }
    }
}

uint32_t TESQuest::GetPriority() {
    return GetData()->ucPriority;
}

void TESQuest::SetStartsEnabled(bool abVal) {
    if (abVal) {
        GetData()->ucFlags |= QUEST_DATA::FLAG_STARTS_ENABLED;
    }
    else {
        GetData()->ucFlags &= ~QUEST_DATA::FLAG_STARTS_ENABLED;
    }
}

void TESQuest::FindCurrentStage() {
    BSSimpleList<TESQuestStage*>* pList = &m_listStages;
    uint8_t ucMaxIndex = 0;
    while (pList && !pList->IsEmpty()) {
        TESQuestStage* pStage = pList->m_item;
        if (pStage->m_Data.bIsDone && pStage->m_Data.ucIndex > static_cast<int32_t>(ucMaxIndex)) {
            ucMaxIndex = pStage->m_Data.ucIndex;
        }
        pList = pList->m_pkNext;
    }
    if (ucMaxIndex > static_cast<int32_t>(cCurrentStage)) {
        cCurrentStage = ucMaxIndex;
    }
}

void TESQuest::ClearStageList() {
    for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList->RemoveHead()) {
        TESQuestStage* pStage = pList->m_item;
        if (pStage) {
            delete pStage;
        }
    }
}

void TESQuest::ClearObjectiveList() {
    for (BSSimpleList<BGSQuestObjective*>* pList = GetObjectiveList(); pList && !pList->IsEmpty();
         pList->RemoveHead()) {
        BGSQuestObjective* pObjective = pList->m_item;
        if (pObjective) {
            delete pObjective;
        }
    }
}

bool TESQuest::GetFailed() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_FAILED) != 0;
}

bool TESQuest::GetCompleted() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_COMPLETED) != 0;
}

uint32_t TESQuest::GetCurrentStage() {
    return cCurrentStage;
}

int32_t TESQuest::GetHasShownQuestUpdate() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_DISPLAYED_QUEST_STARTED) != 0;
}

void TESQuest::SetDisplayedQuestStarted(char acDisplayed) {
    if (acDisplayed) {
        GetData()->ucFlags |= QUEST_DATA::FLAG_DISPLAYED_QUEST_STARTED;
    }
    else {
        GetData()->ucFlags &= ~QUEST_DATA::FLAG_DISPLAYED_QUEST_STARTED;
    }
}

bool TESQuest::GetAllowRepeatStages() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_ALLOW_REPEATED_STAGES) != 0;
}

bool TESQuest::GetAllowRepeats() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_ALLOW_REPEATED_TOPICS) != 0;
}

int32_t TESQuest::GetFlag0x10() {
    return (GetData()->ucFlags & QUEST_DATA::FLAG_STARTS_ENABLED) != 0;
}

void TESQuest::SetCompleted(bool abCompleted) {
    if (!abCompleted || (data.ucFlags & QUEST_DATA::FLAG_COMPLETED) != 0) {
        if (!abCompleted && (data.ucFlags & QUEST_DATA::FLAG_COMPLETED) != 0) {
            data.ucFlags &= ~QUEST_DATA::FLAG_COMPLETED;
            AddChange(CHANGE_QUEST_FLAGS);
            MiscStatManager::Decrement(MiscStatManager::MS_QUESTS_COMPLETED);
        }
    }
    else {
        data.ucFlags |= QUEST_DATA::FLAG_COMPLETED;
        AddChange(CHANGE_QUEST_FLAGS);
        MiscStatManager::Increment(MiscStatManager::MS_QUESTS_COMPLETED);
        HUDMainMenu::ShowQuestName(this);
    }
}

void TESQuest::SetFailed(bool abFailed) {
    if (!abFailed || !GetCompleted()) {
        if (!abFailed || (data.ucFlags & QUEST_DATA::FLAG_FAILED) != 0) {
            if (!abFailed && (data.ucFlags & QUEST_DATA::FLAG_FAILED) != 0) {
                data.ucFlags &= ~QUEST_DATA::FLAG_FAILED;
                AddChange(CHANGE_QUEST_FLAGS);
            }
        }
        else {
            data.ucFlags |= QUEST_DATA::FLAG_FAILED;
            data.ucFlags |= QUEST_DATA::FLAG_COMPLETED;
            AddChange(CHANGE_QUEST_FLAGS);
            HUDMainMenu::ShowQuestName(this);
        }
    }
}

uint8_t TESQuest::IsStageDone(uint8_t aucIndex) {
    for (BSSimpleList<TESQuestStage*>* pList = &m_listStages; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (pList->m_item->m_Data.ucIndex == aucIndex) {
            return pList->m_item->m_Data.bIsDone;
        }
    }
    return 0;
}

void TESQuest::ResetQuest() {
    for (TESQuestStage* pStage : m_listStages) {
        if (pStage) {
            pStage->SetDone(0);
            for (TESQuestStageItem* pStageItem : pStage->listStageItems) {
                Date* pLogDate = pStageItem ? pStageItem->m_pLogDate : nullptr;
                if (pLogDate) {
                    pStageItem->SetLogDate(nullptr);
                    delete pLogDate;
                }
            }
        }
    }
    cCurrentStage = 0;
    for (BGSQuestObjective*& rpItem : GetObjectiveList()->ItemsUntilNull()) {
        rpItem->SetState(QOS_DORMANT);
    }
    if (pScriptLocals) {
        ScriptLocals* pLocals = pScriptLocals;
        if (pLocals) {
            delete pLocals;
        }
        pScriptLocals = pFormScript->CloneLocals();
    }
    TESTopic::ClearAllSaidOnceInfosForQuest(this);
    // The getter's byte is passed on without being normalised to 0 or 1.
    uint8_t ucFlag0x10 = static_cast<uint8_t>(GetFlag0x10());
    SetEnabled(ucFlag0x10);
    SetCompleted(false);
    SetFailed(false);
    SetDisplayedQuestStarted(0);
}

void TESQuest::GetQuestLog(
    BSSimpleList<TESQuestStageItem*>* apListToFill,
    TESQuest* apQuest,
    bool abShowCompleted,
    bool abNewestFirst,
    bool abShowAll
) {
    if (apListToFill) {
        apListToFill->RemoveAll();
        BSSimpleList<TESQuest*> kFoundList;
        BSSimpleList<TESQuestStageItem*>* pList = pPlayer->GetQuestLog();
        while (pList && pList->m_item) {
            TESQuestStageItem* pStageItem = pList->m_item;
            TESQuest* pQuest = pStageItem->GetOwner();
            pList = pList->m_pkNext;
            if (pQuest) {
                bool bShow = !apQuest || pQuest == apQuest;
                if (bShow && !apQuest) {
                    bShow = (abShowCompleted == pQuest->GetCompleted() || abShowAll) && !kFoundList.IsInList(pQuest);
                    if (bShow) {
                        kFoundList.AddHead(pQuest);
                    }
                }
                if (bShow && pStageItem->m_pLogDate && pStageItem->GetLogEntry(pQuest)) {
                    if (abNewestFirst) {
                        apListToFill->AddTail(pStageItem);
                    }
                    else {
                        apListToFill->AddHead(pStageItem);
                    }
                }
            }
        }
        kFoundList.RemoveAll();
    }
}

void BGSQuestObjective::SetOwner(TESQuest* apQuest) {
    pOwnerQuest = apQuest;
}

void TESQuestStageItem::SetLogDate(Date* apDate) {
    m_pLogDate = apDate;
}
