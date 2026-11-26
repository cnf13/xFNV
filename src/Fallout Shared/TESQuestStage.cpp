#include "Fallout Shared/TESQuestStage.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/MessageHandler.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/SCRIPT_REFERENCED_OBJECT.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BSStringT<char> cLogEntry;

// Used by Load: the script variable the SLSD/SCVR chunks being read belong to.
static ScriptVariable* pNewVariable;

TESQuestStageItem::TESQuestStageItem()
  : m_fileOffset(0), ucIndex(0), m_bHasLogEntry(false), m_pLogDate(nullptr), m_pOwner(nullptr), m_pNextQuest(nullptr) {
    ourMemset(&m_Data, 0, 1u);
    cResultScript.SetInitialized(false);
    cResultScript.SetTemporary();
    cResultScript.m_header.bIsQuestScript = true;
}

TESQuestStageItem::~TESQuestStageItem() {
    pCurrentItem = nullptr;
    cLogEntry.Set(nullptr, 0);
    if (m_pLogDate) {
        delete m_pLogDate;
    }
}

void TESQuestStageItem::SetHasLogEntry(bool abHasLogEntry) {
    m_bHasLogEntry = abHasLogEntry;
}

// The parameter is the log entry's index, so a byte and not a bool.
void TESQuestStageItem::SetLogEntryID(uint8_t acIndex) {
    ucIndex = acIndex;
}

// Loads the item's own chunks. The chunk locals are set to 0 because GetChunkData writes nothing for an empty chunk.
// The SLSD/SCVR case re-reads the chunk because TESQuest::Load dispatches the same codes here and passes the chunk it
// read.
bool TESQuestStageItem::Load(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    switch (apFile->GetTESChunk()) {
        case SCHR_ID:
            apFile->GetChunkData(cResultScript.GetHeader(), 0);
            if (apFile->QEndian()) {
                cResultScript.GetHeader()->Endian();
            }
            break;
        case SCDA_ID: {
            uint32_t uiChunkSize = apFile->GetChunkSize();
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
            ourMemset(pBuffer, 0, uiChunkSize);
            apFile->GetChunkData(pBuffer, 0);
            cResultScript.SetCompileData(uiChunkSize, pBuffer);
            break;
        }
        case SLSD_ID:
        case SCVR_ID:
            if (apFile->GetTESChunk() == SLSD_ID) {
                pNewVariable = new ScriptVariable;
                pNewVariable->Load(apFile);
                cResultScript.GetVariableList()->AddTail(pNewVariable);
            }
            else if (pNewVariable) {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                pNewVariable->cName.Set(pBuffer, 0);
            }
            break;
        case SCRO_ID: {
            Script::RefVariable* pReferenced = new Script::RefVariable;
            uint32_t uiFormID = 0;
            apFile->GetChunkData(uiFormID);
            pReferenced->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiFormID));
            cResultScript.GetReferencedObjectList()->AddTail(pReferenced);
            break;
        }
        case SCRV_ID: {
            Script::RefVariable* pReferenced = new Script::RefVariable;
            uint32_t uiVariableID = 0;
            apFile->GetChunkData(uiVariableID);
            pReferenced->varIdx = uiVariableID;
            cResultScript.GetReferencedObjectList()->AddTail(pReferenced);
            break;
        }
        case QSDT_ID:
            m_fileOffset = apFile->GetOffset();
            apFile->GetChunkData(this, 1);
            break;
        default: break;
    }
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != NAM0_ID) {
        return true;
    }
    uint32_t uiNextQuest = 0;
    apFile->GetChunkData(uiNextQuest);
    m_pNextQuest = reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(uiNextQuest));
    return apFile->NextChunk();
}

// Reads the item's own record and takes the log entry whose QSDT index matches ucIndex. The text is kept in
// cLogEntry, which is why the lookup of the same item again skips the file.
const char* TESQuestStageItem::GetLogEntry(TESForm* apOwner) {
    if (!apOwner) {
        return nullptr;
    }
    if (this != pCurrentItem) {
        pCurrentItem = this;
        cLogEntry.Set(nullptr, 0);
        TESFile* pFile = apOwner->GetFile(-1);
        if (!pFile || !m_fileOffset) {
            return nullptr;
        }
        pFile = pFile->GetThreadSafeFile();
        if (!pFile->SetOffset(m_fileOffset)) {
            return nullptr;
        }
        uint8_t cFormType = static_cast<uint8_t>(pFile->GetTESForm());
        if (cFormType != formEnumString[apOwner->GetFormType()].cFormID) {
            return nullptr;
        }
        bool bInEntry = false;
        uint8_t ucEntry = 0;
        for (;;) {
            uint32_t uiChunk = pFile->GetTESChunk();
            if (uiChunk == NO_CHUNK) {
                break;
            }
            if (uiChunk == CNAM_ID) {
                if (bInEntry) {
                    uint32_t uiMaxSize = pFile->GetChunkSize();
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiMaxSize));
                    pFile->GetChunkData(pBuffer, uiMaxSize);
                    cLogEntry.Set(pBuffer, 0);
                }
            }
            else if (uiChunk == QSDT_ID) {
                if (ucEntry == ucIndex) {
                    bInEntry = true;
                }
                ucEntry = static_cast<uint8_t>(ucEntry + 1);
            }
            if (cLogEntry.String()) {
                break;
            }
            if (!pFile->NextChunk()) {
                break;
            }
        }
    }
    // This function returns char* while BSStringT::String returns const char*, hence the const_cast.
    return cLogEntry.String() ? const_cast<char*>(cLogEntry.String()) : nullptr;
}

bool TESQuestStageItem::GetEndQuest() {
    return (m_Data.iFlags & QUEST_STAGE_ITEM_DATA::FLAG_END_QUEST) != 0;
}

bool TESQuestStageItem::GetFailQuest() {
    return (m_Data.iFlags & QUEST_STAGE_ITEM_DATA::FLAG_FAIL_QUEST) != 0;
}

void TESQuestStageItem::Resolve(bool abAlwaysPass) {
    if (m_pLogDate && m_pOwner && !m_pOwner->GetAllowRepeatStages()) {
        Warning("QUESTS: Trying to resolve a quest stage item that already has a log date.");
    }
    else if (abAlwaysPass || objConditions.IsTrue(pPlayer, nullptr)) {
        // The day's byte is sign-extended to the 16-bit value passed to Date.
        m_pLogDate = new Date(
            static_cast<uint16_t>(static_cast<int16_t>(appCalendar.GetDay())),
            appCalendar.GetMonth(),
            appCalendar.GetYear()
        );
        if (GetEndQuest() && m_pOwner) {
            m_pOwner->SetCompleted(true);
            if (m_pOwner == pPlayer->GetActiveQuest()) {
                pPlayer->SetActiveQuest(nullptr);
            }
        }

        if (GetFailQuest() && m_pOwner) {
            m_pOwner->SetFailed(true);
            if (m_pOwner == pPlayer->GetActiveQuest()) {
                pPlayer->SetActiveQuest(nullptr);
            }
        }

        if (m_bHasLogEntry) {
            pPlayer->AddQuestStageItem(this);
        }
        Script* pResultScript = &cResultScript;
        if (Script::GetProcessScripts() && pResultScript && pResultScript->m_header.dataSize) {
            pResultScript->m_header.bIsQuestScript = false;
            ScriptLocals* pScriptLocals = pPlayer->m_Extra.GetScriptLocals();
            pResultScript->Run(pPlayer, pScriptLocals, nullptr, true);
        }
    }
}

void TESQuestStageItem::InitItem(TESForm* apOwner, TESQuestStage* apItem) {
    objConditions.InitItem(apOwner);
    int32_t iPrevWarnings = MessageHandler::QWarningCount();
    MessageHandler::IncDisableWarningCount(true);
    MessageHandler::SetWarningCount(0);
    cResultScript.InitItem(apOwner);
    MessageHandler::IncDisableWarningCount(false);
    int32_t iRefWarnings = MessageHandler::QWarningCount();
    MessageHandler::SetWarningCount(iPrevWarnings);
    if (iRefWarnings) {
        int32_t iFormID = apOwner->iFormID;
        const char* pEditorID = apOwner->GetFormEditorID();
        Warning(
            "MASTERFILE: Errors were encountered during InitItem for result script on quest stage %d, quest '%s' "
            "(%08X)\n\nSee Warnings file for more information.",
            apItem->m_Data.ucIndex,
            pEditorID,
            iFormID
        );
    }
    m_pOwner = dynamic_cast<TESQuest*>(apOwner);
    uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_pNextQuest));
    if (uiTempInt) {
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(uiTempInt, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        TESQuest* pNext = dynamic_cast<TESQuest*>(pForm);
        if (!pNext) {
            Warning("MASTERFILE: Unable to find next quest (%08X) on quest stage.", uiTempInt);
        }
        m_pNextQuest = pNext;
    }
}

void TESQuestStageItem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

QUEST_STAGE_DATA::QUEST_STAGE_DATA() : ucIndex(0), bIsDone(0) {}

TESQuestStage::TESQuestStage() {}

TESQuestStage::~TESQuestStage() {
    ClearStageItemList();
}

// The 8-byte INDX branch copies a local that GetChunkData fills; the local is initialised to 0 (a real 8-byte chunk is
// written whole).
void TESQuestStage::Load(TESFile* apFile) {
    if (apFile && apFile->GetTESChunk() == INDX_ID) {
        if (apFile->GetChunkSize() == 8) {
            QUEST_STAGE_DATA_OLD kOldData = {};
            apFile->GetChunkData(&kOldData, 8);
            m_Data.bIsDone = kOldData.bIsDone;
            m_Data.ucIndex = kOldData.ucIndex;
        }
        else {
            apFile->GetChunkData(&m_Data, 2);
        }
    }
}

TESQuestStageItem* TESQuestStage::GetStageItem(uint8_t acIndex) {
    for (BSSimpleList<TESQuestStageItem*>* pNode = GetStageItemList(); pNode;) {
        TESQuestStageItem* pItem = pNode->GetItem();
        if (pItem && pItem->GetIndex() == acIndex) {
            return pItem;
        }
        pNode = pNode->GetNext();
    }
    return nullptr;
}

// Steps to the next node before resolving the item.
void TESQuestStage::SetDone(TESQuest* apQuest, bool abAllItems) {
    if (GetDone() && !(apQuest && apQuest->GetAllowRepeatStages())) {
        return;
    }
    m_Data.bIsDone = true;
    for (BSSimpleList<TESQuestStageItem*>* pNode = GetStageItemList(); pNode;) {
        TESQuestStageItem* pItem = pNode->GetItem();
        pNode = pNode->GetNext();
        if (pItem) {
            pItem->Resolve(abAllItems);
        }
    }
}

void TESQuestStage::SetDone(uint8_t abDone) {
    m_Data.bIsDone = abDone;
}

// Deletes every item of the stage and empties its list. Its one caller is the destructor.
void TESQuestStage::ClearStageItemList() {
    for (BSSimpleList<TESQuestStageItem*>* pList = &listStageItems; pList && !pList->IsEmpty(); pList->RemoveHead()) {
        TESQuestStageItem* pItem = pList->m_item;
        if (pItem) {
            delete pItem;
        }
    }
}

void TESQuestStage::InitItem(TESForm* apOwner) {
    BSSimpleList<TESQuestStageItem*>* pList = &listStageItems;
    while (pList) {
        TESQuestStageItem* pItem = pList->m_item;
        if (pItem) {
            pItem->InitItem(apOwner, this);
            pItem->cResultScript.m_header.bIsQuestScript = false;
        }
        pList = pList->m_pkNext;
    }
}

void TESQuestStage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
