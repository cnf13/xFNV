#include "Fallout Shared/TESScript.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/SCRIPT_REFERENCED_OBJECT.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptRunManager.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <cstring>

// Script::InitItem's function-local statics: the quest scripts initialized so far, and the halvings of
// fQuestScriptDelayTime whose sums spread their first runs.
using QuestDelayTimes = float[8];
static QuestDelayTimes fdelayTimes{};
static int32_t iquestScriptCount;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fQuestScriptDelayTime("fQuestScriptDelayTime:MAIN", 5.0f);
BSCriticalSection Script::ScriptRefListCrit;
BSSimpleList<TESObjectREFR*> Script::ReferencesToDisable;
BSSimpleList<TESObjectREFR*> Script::ReferencesToEnable;
BSSimpleList<TESObjectREFR*> Script::ReferencesToDelete;
BSSimpleList<TESObjectREFR*> Script::ReferencesToFadeDisable;

ScriptLocals::ScriptLocals() {
    // Zeroes the whole object, padding included, as the game does, so the three bytes after m_cFlags do not carry heap
    // garbage. Harmless: the class has no vtable and its members are plain pointers and bytes, set just after.
    memset(static_cast<void*>(this), 0, sizeof(ScriptLocals));

    m_pMasterScript = nullptr;
    m_cFlags = 0;
    m_pActionList = nullptr;
    m_pLocalList = nullptr;
    m_pScriptEffectData = nullptr;
}

ScriptLocals::~ScriptLocals() {
    ClearActionList();
    ClearVariableList();
    if (m_pScriptEffectData) {
        ourFree(m_pScriptEffectData);
    }
}

uint16_t ScriptLocals::GetSaveSize() {
    int16_t v7 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v7 = 6;
    }
    uint16_t isize = v7 + 2;
    for (SCRIPT_LOCAL* plocal : *m_pLocalList) {
        if ((plocal) && (0.0 != plocal->fValue)) {
            if (IsReferenceVariable(plocal->uiID)) {
                isize += 8;
            }
            else {
                isize += 12;
            }
        }
    }
    uint16_t isize_2 = isize + 1;
    if (m_pScriptEffectData) {
        isize_2 += 8;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize_2,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 638,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize_2, 638,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp"
            );
        }
    }
    return isize_2;
}

// Takes the next node before it clears the action's flags.
void ScriptLocals::ClearActionFlags() {
    BSSimpleList<ACTION_OBJECT*>* pList = m_pActionList;
    while (pList != nullptr && pList->GetItem() != nullptr) {
        ACTION_OBJECT* pAction = pList->GetItem();
        pList = pList->GetNext();
        pAction->iFlags = 0;
    }
}

bool ScriptLocals::CheckActionFlag(TESForm* apActionForm, uint32_t aiFlag) {
    ACTION_OBJECT* pAction = GetActionObject(apActionForm, false);
    if (pAction == nullptr) {
        AddActionObject(apActionForm);
        return false;
    }
    return (pAction->iFlags & aiFlag) != 0;
}

bool ScriptLocals::SetActionFlag(TESForm* apActionForm, uint32_t aiFlag) {
    if (m_pActionList == nullptr) {
        return false;
    }

    bool bResult = false;
    ACTION_OBJECT* pAction = GetActionObject(apActionForm, true);
    if (pAction != nullptr) {
        pAction->iFlags |= aiFlag;
        bResult = true;
    }
    // The action whose form is not known yet is flagged as well.
    ACTION_OBJECT* pDefaultAction = GetActionObject(nullptr, true);
    if (pDefaultAction != nullptr) {
        pDefaultAction->iFlags |= aiFlag;
        bResult = true;
    }
    return bResult;
}

ACTION_OBJECT* ScriptLocals::GetActionObject(TESForm* apActionForm, bool abReturnDefault) {
    if (m_pActionList == nullptr) {
        return nullptr;
    }

    // The engine keeps the last action whose form is still unknown, for callers that add it later.
    ACTION_OBJECT* pDefaultAction = nullptr;
    for (BSSimpleList<ACTION_OBJECT*>* pList = m_pActionList; pList != nullptr && pList->GetItem() != nullptr;
         pList = pList->GetNext()) {
        ACTION_OBJECT* pAction = pList->GetItem();
        if (pAction->pForm == apActionForm) {
            return pAction;
        }
        if (pAction->pForm == nullptr) {
            pDefaultAction = pAction;
        }
    }
    if (abReturnDefault) {
        return pDefaultAction;
    }
    return nullptr;
}

void ScriptLocals::ClearActionList() {
    if (m_pActionList == nullptr) {
        return;
    }

    while (m_pActionList->GetItem() != nullptr) {
        ACTION_OBJECT* pAction = m_pActionList->GetItem();
        m_pActionList->RemoveHead();
        ourFree(pAction);
    }
    delete m_pActionList;
    m_pActionList = nullptr;
}

void ScriptLocals::ClearVariableList() {
    if (m_pLocalList == nullptr) {
        return;
    }

    while (m_pLocalList->GetItem() != nullptr) {
        SCRIPT_LOCAL* pVariable = m_pLocalList->GetItem();
        m_pLocalList->RemoveHead();
        ourFree(pVariable);
    }
    delete m_pLocalList;
    m_pLocalList = nullptr;
}

void ScriptLocals::AddActionObject(TESForm* apActionForm) {
    alignas(AutoMemContext) uint8_t contextStorage[sizeof(AutoMemContext)];
    AutoMemContext* pContext = ::new (contextStorage) AutoMemContext(
        MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 300
    );

    if (m_pActionList == nullptr) {
        void* pMemory = ourMalloc(sizeof(BSSimpleList<ACTION_OBJECT*>));
        m_pActionList = pMemory != nullptr ? ::new (pMemory) BSSimpleList<ACTION_OBJECT*> : nullptr;
    }

    ACTION_OBJECT* pAction = GetActionObject(apActionForm, false);
    if (pAction == nullptr) {
        pAction = static_cast<ACTION_OBJECT*>(ourMalloc(sizeof(ACTION_OBJECT)));
        pAction->pForm = apActionForm;
        pAction->iFlags = 0;
        m_pActionList->AddHead(pAction);
    }

    pContext->~AutoMemContext();
}

double ScriptLocals::GetVariable(uint32_t auiID, Script* apScript) {
    const char* v4;
    SCRIPT_LOCAL* plocalvar = nullptr;
    if (GetThreadTLSData()->pLastScriptEventList == this && GetThreadTLSData()->uiLastScriptVarID == auiID) {
        plocalvar = GetThreadTLSData()->pLastScriptVariable;
    }
    if (!plocalvar) {
        for (BSSimpleList<SCRIPT_LOCAL*>* plist = m_pLocalList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            SCRIPT_LOCAL* m_item = plist->m_item;
            if (m_item->uiID == auiID) {
                plocalvar = m_item;
                break;
            }
        }
    }
    GetThreadTLSData()->pLastScriptEventList = this;
    GetThreadTLSData()->uiLastScriptVarID = auiID;
    GetThreadTLSData()->pLastScriptVariable = plocalvar;
    if (plocalvar) {
        return plocalvar->fValue;
    }
    if (apScript) {
        v4 = apScript->GetFormEditorID();
    }
    else {
        v4 = "UNKNOWN";
    }
    Warning(
        "SCRIPTS: Variable ID %08X not found. Try to recompile script '%s'. The script may also have a bad "
        "reference variable in an if statement on line %d.",
        auiID, v4, iCurrentScriptLine
    );
    return 0.0;
}

void ScriptLocals::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    ClearVariableList();
    ClearActionFlags();
    if (m_pMasterScript) {
        m_pLocalList = m_pMasterScript->CreateLocalList();
    }
    if (m_pScriptEffectData) {
        ourFree(m_pScriptEffectData);
    }
    m_pScriptEffectData = nullptr;
}

void ScriptLocals::SaveGame() {
    uint32_t irefID;
    uint32_t ivarID;
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    bool bhasScriptEffectData;
    iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    for (SCRIPT_LOCAL* plocal : *m_pLocalList) {
        if ((plocal) && (0.0 != plocal->fValue)) {
            if (IsReferenceVariable(plocal->uiID)) {
                irefID = 0;
                Script::GetNumericIDFromDouble(irefID, plocal->fValue);
                ivarID = plocal->uiID | 0x80000000u;
                pSaveLoadGameOLD->SaveGameDataOLD(&ivarID, 4u);
                pSaveLoadGameOLD->SaveNumericID(&irefID, 4u);
            }
            else {
                pSaveLoadGameOLD->SaveGameDataOLD(plocal, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&plocal->fValue, 8u);
            }
            ++icount;
        }
    }
    *pcount = icount;
    bhasScriptEffectData = m_pScriptEffectData != nullptr;
    pSaveLoadGameOLD->SaveGameDataOLD(&bhasScriptEffectData, 1u);
    if (bhasScriptEffectData) {
        pSaveLoadGameOLD->SaveGameDataOLD(m_pScriptEffectData, 8u);
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
                BufferPointer - itempStartPosition, pheader->iFormID, v1, iFlags, 694,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition, 694,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block size counts the bytes from its own field to the buffer's end; the save buffer is raw bytes, and the
        // addresses are compared unsigned.
        uint8_t* pBufferEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = reinterpret_cast<uint8_t*>(pblockSize);
        if (pBufferEnd > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 694
            );
        }
        *pblockSize = static_cast<uint16_t>(pBufferEnd - pBlockStart);
    }
}

void ScriptLocals::LoadGame() {
    uint32_t irefID;
    uint32_t ivarID;
    double fvalue;
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    bool bhasScriptEffectData;
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 700, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 700,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
    for (int32_t i = 0; i < icount; ++i) {
        ivarID = 0;
        fvalue = 0.0;
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 117u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&ivarID, 4u);
            if ((ivarID & 0x80000000u) == 0) {
                pSaveLoadGameOLD->LoadGameDataOLD(&fvalue, 8u);
            }
            else {
                ivarID &= ~0x80000000u;
                pSaveLoadGameOLD->LoadNumericID(&irefID, 4u);
                Script::PutNumericIDInDouble(irefID, fvalue);
            }
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() < 117u) {
            pSaveLoadGameOLD->LoadGameDataOLD(&ivarID, 4u);
            pSaveLoadGameOLD->LoadGameDataOLD(&fvalue, 8u);
        }
        SetVariable(ivarID, fvalue);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&bhasScriptEffectData, 1u);
    if (bhasScriptEffectData) {
        m_pScriptEffectData = static_cast<SCRIPT_EFFECT_DATA*>(ourMalloc(sizeof(SCRIPT_EFFECT_DATA)));
        pSaveLoadGameOLD->LoadGameDataOLD(m_pScriptEffectData, 8u);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t value_4a = pheader_2->iFlags;
                    int32_t valuea = pheader_2->cVersion;
                    const char* v2 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 747,
                        pheader_2->iFormID, v2, valuea, value_4a
                    );
                }
            }
            else {
                uint32_t value_4 = pheader_2->iFlags;
                int32_t value = pheader_2->cVersion;
                const char* v1 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 747, pheader_2->iFormID,
                    v1, value, value_4
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t value_4c = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 747, value_4c
                    );
                }
            }
            else {
                int32_t value_4b = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 747, value_4b
                );
            }
        }
    }
}

void ScriptLocals::SaveGame(BGSSaveFormBuffer* apBuffer) {
    uint32_t ivarID;
    bool bhasScriptEffectData;
    bool bonLoadFlagSet;
    uint32_t icount = 0;
    uint32_t isaveGameBufferPos = apBuffer->StartVariableSizedValue();
    for (SCRIPT_LOCAL* plocal : *m_pLocalList) {
        if ((plocal) && (0.0 != plocal->fValue)) {
            if (IsReferenceVariable(plocal->uiID)) {
                uint32_t irefID = 0;
                Script::GetNumericIDFromDouble(irefID, plocal->fValue);
                ivarID = plocal->uiID | 0x80000000u;
                apBuffer->SaveDataEndian(&ivarID, 4u, 0);
                apBuffer->SaveFormID(irefID, 0);
            }
            else {
                apBuffer->SaveDataEndian(plocal, 4u, 0);
                apBuffer->SaveDataEndian(&plocal->fValue, 8u, 0);
            }
            ++icount;
        }
    }
    apBuffer->SaveVariableSizedValue(icount, isaveGameBufferPos);
    bhasScriptEffectData = m_pScriptEffectData != nullptr;
    apBuffer->SaveDataEndian(&bhasScriptEffectData, 1u, 0);
    if (bhasScriptEffectData) {
        apBuffer->SaveDataEndian(m_pScriptEffectData, 8u, 0);
    }
    bonLoadFlagSet = CheckActionFlag(nullptr, SCRIPT_ACTION_ONLOAD);
    apBuffer->SaveDataEndian(&bonLoadFlagSet, 1u, 0);
}

void ScriptLocals::SetVariable(uint32_t auiID, double value) {
    BSSimpleList<SCRIPT_LOCAL*>* pList = m_pLocalList;
    while (pList && pList->m_item) {
        SCRIPT_LOCAL* pLocal = pList->m_item;
        pList = pList->m_pkNext;
        if (pLocal->uiID == auiID) {
            pLocal->fValue = value;
            return;
        }
    }
    bool bshowGenericWarning = true;
    if (m_pMasterScript) {
        Warning(
            "SCRIPTS: Trying to set variableID %d in script '%s' -- variable not found.", auiID,
            m_pMasterScript->GetFormEditorID()
        );
        bshowGenericWarning = false;
    }
    else {
        if (pSaveLoadGameOLD->GetLoading()) {
            LoadFormHeader* ploadHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (ploadHeader) {
                const char* pfullname;
                TESForm* pform = TESForm::GetFormByNumericID(ploadHeader->iFormID);
                TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(pform);
                if (pref) {
                    pfullname = pref->GetFullName();
                }
                else {
                    pfullname = TESFullName::GetFullName(pform);
                }
                if (pform && pfullname) {
                    if (pref) {
                        Warning(
                            "SAVELOAD: (LoadGame '%s') Trying to set variableID %d on ref '%s' (%08X) -- variable "
                            "not found. The script may have been changed in the master/plug-in file.",
                            pSaveLoadGameOLD->GetMostRecentSaveGame(), auiID, pfullname, ploadHeader->iFormID
                        );
                    }
                    else {
                        Warning(
                            "SAVELOAD: (LoadGame '%s') Trying to set variableID %d on '%s' (%08X) -- variable not "
                            "found. The script may have been changed in the master/plug-in file.",
                            pSaveLoadGameOLD->GetMostRecentSaveGame(), auiID, pfullname, ploadHeader->iFormID
                        );
                    }
                }
                else {
                    Warning(
                        "SAVELOAD: (LoadGame '%s') Trying to set variableID %d on (%08X) -- variable not found. "
                        "The script may have been changed in the master/plug-in file.",
                        pSaveLoadGameOLD->GetMostRecentSaveGame(), auiID, ploadHeader->iFormID
                    );
                }
                bshowGenericWarning = false;
            }
        }
    }
    if (bshowGenericWarning) {
        Warning("SCRIPTS: Trying to set variableID %d -- variable not found.", auiID);
    }
}

bool ScriptLocals::IsReferenceVariable(uint32_t auiID) {
    if (!m_pMasterScript) {
        return false;
    }
    for (Script::RefVariable*& rpItem : (&m_pMasterScript->listRefObjects)->ItemsUntilNull()) {
        if (rpItem->varIdx == auiID) {
            return true;
        }
    }
    return false;
}

// Replaces the actions with copies of apCopy's.
void ScriptLocals::CopyActionList(ScriptLocals* pCopy) {
    if (m_pActionList) {
        ClearActionList();
    }
    if (!pCopy) {
        return;
    }
    BSSimpleList<ACTION_OBJECT*>* pCopyList = pCopy->m_pActionList;
    if (!pCopyList || pCopyList->IsEmpty()) {
        return;
    }
    BSSimpleList<ACTION_OBJECT*>* pList = m_pActionList;
    while (pCopyList && pCopyList->m_item) {
        ACTION_OBJECT* pCopyAction = pCopyList->m_item;
        ACTION_OBJECT* pAction = static_cast<ACTION_OBJECT*>(ourMalloc(sizeof(ACTION_OBJECT)));
        pAction->pForm = pCopyAction->pForm;
        pAction->iFlags = pCopyAction->iFlags;
        if (pList) {
            pList->AddTail(pAction);
            pList = pList->m_pkNext;
        }
        else {
            void* pMemory = ourMalloc(sizeof(BSSimpleList<ACTION_OBJECT*>));
            m_pActionList = pMemory ? ::new (pMemory) BSSimpleList<ACTION_OBJECT*> : nullptr;
            m_pActionList->AddHead(pAction);
            pList = m_pActionList;
        }
        pCopyList = pCopyList->m_pkNext;
    }
}

void ScriptLocals::Revert_0() {
    ClearVariableList();
    ClearActionFlags();
    if (m_pMasterScript) {
        m_pLocalList = m_pMasterScript->CreateLocalList();
    }
    if (m_pScriptEffectData) {
        ourFree(m_pScriptEffectData);
    }
    m_pScriptEffectData = nullptr;
}

void ScriptLocals::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t irefID;
    uint32_t ivarID;
    double fvalue;
    bool bhasScriptEffectData;
    bool bonLoadFlagSet;
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        fvalue = 0.0;
        ivarID = 0;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&ivarID), 4u);
        if ((ivarID & 0x80000000u) == 0) {
            apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fvalue), 8u);
        }
        else {
            ivarID &= ~0x80000000u;
            irefID = 0;
            apLoadGameBuffer->LoadFormID(irefID);
            Script::PutNumericIDInDouble(irefID, fvalue);
        }
        SetVariable(ivarID, fvalue);
    }
    bhasScriptEffectData = false;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bhasScriptEffectData), 1u);
    if (bhasScriptEffectData) {
        m_pScriptEffectData = static_cast<SCRIPT_EFFECT_DATA*>(ourMalloc(sizeof(SCRIPT_EFFECT_DATA)));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&m_pScriptEffectData->m_bScriptEffectStart), 8u);
    }
    bonLoadFlagSet = false;
    if (apLoadGameBuffer->GetVersion() >= 21u) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bonLoadFlagSet), 1u);
    }
    if (m_pActionList) {
        ClearActionFlags();
    }
    else {
        TESObjectREFR* value = apLoadGameBuffer->GetReference();
        Script* m_pMasterScript = this->m_pMasterScript;
        ScriptRunManager* v2 = ScriptRunManager::Instance();
        v2->InitActionList(m_pMasterScript, value, this);
    }
    if (bonLoadFlagSet) {
        SetActionFlag(nullptr, SCRIPT_ACTION_ONLOAD);
    }
}

void ScriptLocals::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Forgets the per-thread last script variable lookup in the thread's TLSData block. Each store re-reads the block, as
// the game does.
void ScriptLocals::ClearOptimizations() {
    GetThreadTLSData()->pLastScriptEventList = nullptr;
    GetThreadTLSData()->uiLastScriptVarID = 0xFFFFFFFF;
    GetThreadTLSData()->pLastScriptVariable = nullptr;
}

void* ourCalloc(uint32_t aNum, uint32_t aSize) {
    uint32_t Size = aNum * aSize;
    MemoryManager* v2 = &MemoryManager::Instance();
    void* apDest = v2->Allocate(Size);
    ourMemset(apDest, 0, Size);
    return apDest;
}

bool BSFadeNode::IsVisible() {
    return static_cast<uint8_t>(GetIgnoreFade()) || (!BSFadeNode::bFadeEnabled || fCurrentFade > 0.0);
}

Script::RefVariable::RefVariable() : form(nullptr), varIdx(0) {}

Script::RefVariable::RefVariable(const RefVariable* apSource) : form(apSource->form), varIdx(apSource->varIdx) {
    name.Copy(apSource->name);
}

Script::~Script() {
    Script::ClearData();
}

void Script::InitializeData() {
    ourMemset(&m_header, 0, sizeof(SCRIPT_HEADER));
    m_data = nullptr;
    m_text = nullptr;
    GetThreadSystemOutput() = false;
    pOwnerQuest = nullptr;
    fProfilerTimer = 0.0f;
    fQuestScriptDelay = 0.0f;
    fQuestScriptGetSecondsBuffer = 0.0f;
    SetFormType(SCPT_ID);
}

// SCPT. SLSD adds a new local variable and SCVR names the last one added; SCRO and SCRV add a referenced form or
// variable. A script without compiled data gets a warning. When a chunk is empty (malformed data),
// TESFile::GetChunkData returns without writing, so the SCRO/SCRV value is initialised to 0 and the EDID and SCVR
// buffers to "" (one zeroed byte, allocated only for an empty chunk).
bool Script::Load(TESFile* pFile) {
    ScriptVariable* pVariable = nullptr;
    if (pFile->GetTESForm() != SCPT_ID) {
        return false;
    }
    LoadForm(pFile);
    do {
        uint32_t uiChunk = pFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = pFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten, so it is cleared first (see above).
                pBuffer[0] = '\0';
                pFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(pFile); break;
            case SCHR_ID:
                pFile->GetChunkData(&m_header, 0);
                if (pFile->QEndian()) {
                    m_header.Endian();
                }
                break;
            case SLSD_ID: {
                void* pMemory = ourMalloc(sizeof(ScriptVariable));
                pVariable = pMemory ? ::new (pMemory) ScriptVariable : nullptr;
                pVariable->Load(pFile);
                GetVariableList()->AddTail(pVariable);
                break;
            }
            case SCVR_ID:
                if (pVariable) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    uint32_t uiChunkSize = pFile->GetChunkSize();
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                    // An empty chunk leaves the buffer unwritten, so it is cleared first (see above).
                    pBuffer[0] = '\0';
                    pFile->GetChunkData(pBuffer, 0x200);
                    pVariable->cName.Set(pBuffer, 0);
                }
                break;
            case SCDA_ID: {
                uint32_t uiSize = pFile->GetChunkSize();
                m_data = static_cast<char*>(ourCalloc(1, uiSize));
                ourMemset(m_data, 0, uiSize);
                pFile->GetChunkData(m_data, 0);
                break;
            }
            case SCRO_ID:
            case SCRV_ID: {
                void* pMemory = ourMalloc(sizeof(RefVariable));
                RefVariable* pReference = pMemory ? ::new (pMemory) RefVariable : nullptr;
                uint32_t uiData = 0;
                pFile->GetChunkData(uiData);
                if (uiChunk == SCRO_ID) {
                    // The form ID stands in for the form until the forms are resolved.
                    pReference->form = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiData));
                }
                else {
                    pReference->varIdx = uiData;
                }
                GetReferencedObjectList()->AddTail(pReference);
                break;
            }
            case RNAM_ID: {
                // Read and discarded.
                uint32_t uiUnused = 0;
                pFile->GetChunkData(uiUnused);
                break;
            }
            default: break;
        }
    }
    while (pFile->NextChunk());
    if (!m_data) {
        const char* pFilename = pFile->GetFilename();
        const char* pEditorID = GetFormEditorID();
        Warning("MASTERFILE: Script '%s' in file '%s' has not been compiled.\r\n", pEditorID, pFilename);
    }
    return true;
}

void Script::Save() {
    TESForm::StartForm();
    Script::EndianData();
    if (TESForm::QEndianSwapOnSave()) {
        (&m_header)->Endian();
    }
    TESForm::__AddChunkData(static_cast<CHUNK_ID>(1380467539), &m_header, 0x14u);
    if (TESForm::QEndianSwapOnSave()) {
        (&m_header)->Endian();
    }
    TESForm::CloseForm();
}

bool Script::SaveEdit(TESFile* pFile) {
    return (bool)((uint8_t)TESForm::Save(pFile));
}

// Constructs and destroys an AutoMemContext in local storage and ignores the constructor's result.
void Script::InitItem(TESForm* apOwner) {
    if (GetInitialized() || apOwner == nullptr) {
        return;
    }
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 2741
    );
    if (apOwner != this && GetTemporary()) {
        SetFile(apOwner->GetFile(-1));
    }
    int32_t icount = 0;
    BSSimpleList<RefVariable*>* pList = GetReferencedObjectList();
    BSSimpleList<RefVariable*>* pLastList = nullptr;
    TESFile* pownerFile = apOwner != nullptr ? apOwner->GetFile(-1) : nullptr;
    while (pList != nullptr && !pList->IsEmpty()) {
        RefVariable* pRefObject = pList->GetItem();
        // until the script is initialized, a reference's form field holds the form ID Load read
        uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefObject->form));
        uint32_t varID = pRefObject->varIdx;
        bool bValid = false;
        if (tempID != 0) {
            AddCompileIndex(tempID, pownerFile);
            pRefObject->form = GetFormByNumericID(tempID);
            if (pRefObject->form != nullptr) {
                bValid = true;
            }
        }
        else if (varID != 0) {
            bValid = true;
        }
        if (bValid) {
            pLastList = pList;
            pList = pList->GetNext();
        }
        else {
            if (pLastList != nullptr) {
                pLastList->Remove(pList->GetItem());
                pList = pLastList->GetNext();
            }
            else {
                pList->RemoveHead();
            }
            const char* pEditorID = GetFormEditorID();
            uint32_t uiFormID = GetFormID();
            if (tempID != 0) {
                Warning(
                    "MASTERFILE: Could not find referenced object (%08X) on Script (%08X) '%s'.  Script will not "
                    "be executed.",
                    tempID, uiFormID, pEditorID
                );
            }
            else {
                Warning(
                    "MASTERFILE: Referenced object %d on Script (%08X) '%s' is invalid.  Script will not be "
                    "executed.",
                    icount, uiFormID, pEditorID
                );
            }
            m_header.dataSize = 0;
        }
        ++icount;
    }
    if (static_cast<uint32_t>(icount) != m_header.refObjectCount && m_header.dataSize != 0) {
        uint32_t uiNumRefs = m_header.refObjectCount;
        const char* pEditorID = GetFormEditorID();
        uint32_t uiFormID = GetFormID();
        Warning(
            "MASTERFILE: Referenced object list on Script (%08X) '%s' is corrupt.  Expected %d objects, found %d.",
            uiFormID, pEditorID, uiNumRefs, icount
        );
        m_header.dataSize = 0;
    }
    if (m_data != nullptr) {
        EndianData();
    }
    if (GetHeader()->bIsQuestScript && fQuestScriptDelayTime.Float() > 0.0) {
        float ftmpTime = fQuestScriptDelayTime.Float();
        if (iquestScriptCount == 0) {
            for (float& fDelayTime : fdelayTimes) {
                ftmpTime = static_cast<float>(ftmpTime / 2.0);
                fDelayTime = ftmpTime;
            }
        }
        // the low 8 bits of the count pick which halvings this script's first delay adds up
        int32_t iquestSpread = iquestScriptCount & 0xFF;
        int32_t iquestIndex = 0;
        if (pOwnerQuest != nullptr && pOwnerQuest->GetScriptProcessingDelay() > 0.0) {
            fQuestScriptDelay = 0.0f;
        }
        else if (iquestSpread == 0) {
            fQuestScriptDelay = fQuestScriptDelayTime.Float();
        }
        else {
            while (iquestSpread != 0) {
                if (iquestSpread & 1) {
                    fQuestScriptDelay = fQuestScriptDelay + fdelayTimes[iquestIndex];
                }
                ++iquestIndex;
                iquestSpread >>= 1;
            }
        }
        ++iquestScriptCount;
    }
    if (apOwner->GetFormType() == QUST_ID) {
        pOwnerQuest = static_cast<TESQuest*>(apOwner);
    }
    SetInitialized(true);
    pContext->~AutoMemContext();
}

void Script::ClearAllReferenceObjects() {
    if (GetThreadLastRefSearchScript() == this) {
        GetThreadLastRefSearchScript() = nullptr;
    }
    BSSimpleList<RefVariable*>* pNode = GetReferencedObjectList();
    while (pNode != nullptr && !pNode->IsEmpty()) {
        RefVariable* pRefVariable = pNode->GetItem();
        if (pRefVariable != nullptr) {
            delete pRefVariable;
        }
        pNode->RemoveHead();
    }
}

// The lists are fetched destination first, as the game does; the text getter is called twice.
void Script::CopyResultScript(Script* apScriptToCopy) {
    if (!apScriptToCopy) {
        return;
    }
    ourMemcpy(&m_header, apScriptToCopy->GetHeader(), sizeof(SCRIPT_HEADER));
    m_header.variableCount = 0;
    char* pCompiledData = apScriptToCopy->GetCompiledData();
    SetCompileData(m_header.dataSize, pCompiledData);
    ClearAllReferenceObjects();
    BSSimpleList<RefVariable*>* pToRefs = GetReferencedObjectList();
    BSSimpleList<RefVariable*>* pFromRefs = apScriptToCopy->GetReferencedObjectList();
    CopyReferencedObjectList(pFromRefs, pToRefs, this);
    ClearVariableList();
    BSSimpleList<ScriptVariable*>* pToVars = GetVariableList();
    BSSimpleList<ScriptVariable*>* pFromVars = apScriptToCopy->GetVariableList();
    CopyVariableList(pFromVars, pToVars, this);
    if (apScriptToCopy->GetText()) {
        SetText(apScriptToCopy->GetText());
    }
    else {
        SetText(nullptr);
    }
    SetInitialized(apScriptToCopy->GetInitialized());
}

void Script::ClearLocalVariableRefObjects() {
    if (GetThreadLastRefSearchScript() == this) {
        GetThreadLastRefSearchScript() = nullptr;
    }
    BSSimpleList<RefVariable*>* pList = GetReferencedObjectList();
    while (pList != nullptr && !pList->IsEmpty()) {
        RefVariable* pRefObject = pList->GetItem();
        // a reference held in a local variable drops the form it last resolved to
        if (pRefObject != nullptr && pRefObject->varIdx != 0) {
            pRefObject->form = nullptr;
        }
        pList = pList->GetNext();
    }
}

void Script::EndianData() {}

Script::RefVariable* Script::GetReferencedObject(uint32_t aiIndex, ScriptLocals* apScriptLocals) {
    if (aiIndex == 0) {
        return nullptr;
    }
    if (aiIndex > m_header.refObjectCount) {
        return nullptr;
    }
    if (GetThreadLastRefSearchScript() == this && GetThreadLastRefSearchIndex() == aiIndex &&
        GetThreadLastScriptLocals() == apScriptLocals) {
        return GetThreadLastRefObject();
    }
    uint32_t uiIndex = 1;
    BSSimpleList<RefVariable*>* pList = GetReferencedObjectList();
    while (pList != nullptr && !pList->IsEmpty() && uiIndex < aiIndex) {
        ++uiIndex;
        pList = pList->GetNext();
    }
    if (pList == nullptr) {
        return nullptr;
    }
    RefVariable* pRefObject = pList->GetItem();
    // a reference held in a local variable resolves to the form whose ID the variable holds
    if (pRefObject->varIdx != 0 && apScriptLocals != nullptr) {
        double dValue = apScriptLocals->GetVariable(pRefObject->varIdx, this);
        uint32_t uiFormID = 0;
        GetNumericIDFromDouble(uiFormID, dValue);
        if (uiFormID != 0) {
            pRefObject->form = GetFormByNumericID(uiFormID);
        }
    }
    GetThreadLastRefSearchScript() = this;
    GetThreadLastScriptLocals() = apScriptLocals;
    GetThreadLastRefSearchIndex() = aiIndex;
    GetThreadLastRefObject() = pRefObject;
    return pRefObject;
}

// Constructs and destroys an AutoMemContext in local storage and ignores the constructor's result.
BSSimpleList<SCRIPT_LOCAL*>* Script::CreateLocalList() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 3307
    );
    void* pMemory = ourMalloc(sizeof(BSSimpleList<SCRIPT_LOCAL*>));
    auto* pList = pMemory ? ::new (pMemory) BSSimpleList<SCRIPT_LOCAL*> : nullptr;
    BSSimpleList<ScriptVariable*>* pIter = GetVariableList();
    while (pIter != nullptr && pIter->GetItem() != nullptr) {
        ScriptVariable* pVariable = pIter->GetItem();
        pIter = pIter->GetNext();
        void* pLocalMemory = ourMalloc(sizeof(SCRIPT_LOCAL));
        SCRIPT_LOCAL* pLocal = pLocalMemory ? ::new (pLocalMemory) SCRIPT_LOCAL(&pVariable->data) : nullptr;
        pList->AddTail(pLocal);
    }
    pContext->~AutoMemContext();
    return pList;
}

BSSimpleList<ScriptVariable*>* Script::GetVariableList() {
    return &listVariables;
}

char* Script::GetCompiledData() {
    return m_data;
}

bool Script::NeedsUpdate(ScriptCompileData* apCompileData) {
    if ((GetFormEditorIDLength()) && (apCompileData->cScriptName.GetLength())) {
        char* v6 = apCompileData->cScriptName.pString;
        const char* v2 = GetFormEditorID();
        if (BSstricmp(v2, v6)) {
            return true;
        }
    }

    if (m_header.dataSize != apCompileData->header.dataSize) {
        return true;
    }
    if (m_header.refObjectCount != apCompileData->header.refObjectCount) {
        return true;
    }
    for (uint32_t n = 0; n < m_header.dataSize; ++n) {
        if (m_data[n] != apCompileData->pOutput[n]) {
            return true;
        }
    }
    BSSimpleList<ScriptVariable*>* pNewList = &apCompileData->listVariables;
    BSSimpleList<ScriptVariable*>* pOldList = GetVariableList();
    int32_t v4 = pNewList->ItemsInList();
    if (v4 != pOldList->ItemsInList()) {
        return true;
    }
    while (pNewList && pOldList) {
        ScriptVariable* pold = pOldList->m_item;
        ScriptVariable* pnew = pNewList->m_item;
        if (pnew && (pold && pnew->Compare(pold))) {
            return true;
        }
        pNewList = pNewList->m_pkNext;
        pOldList = pOldList->m_pkNext;
    }
    BSSimpleList<RefVariable*>* pNewList_2 = &apCompileData->listRefObjects;
    BSSimpleList<RefVariable*>* pOldList_2 = &listRefObjects;
    int32_t v5 = pNewList_2->ItemsInList();
    if (v5 != pOldList_2->ItemsInList()) {
        return true;
    }
    while (pNewList_2 && pOldList_2) {
        RefVariable* pold_2 = pOldList_2->m_item;
        RefVariable* pnew_2 = pNewList_2->m_item;
        if (pnew_2 && (pold_2 && pnew_2->form != pold_2->form)) {
            return true;
        }
        pNewList_2 = pNewList_2->m_pkNext;
        pOldList_2 = pOldList_2->m_pkNext;
    }
    return false;
}

// AutoMemContext as in InitItem.
ScriptLocals* Script::CloneLocals() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 3294
    );
    void* pMemory = ourMalloc(sizeof(ScriptLocals));
    ScriptLocals* pLocals = pMemory ? ::new (pMemory) ScriptLocals : nullptr;
    pLocals->m_pLocalList = CreateLocalList();
    pLocals->m_pMasterScript = this;
    pContext->~AutoMemContext();
    return pLocals;
}

bool Script::Run(
    TESObjectREFR* ownerObject,
    ScriptLocals* ownerVars,
    TESObjectREFR* ownerContainer,
    bool abIsPartialScript
) {
    // a script of at most 4 bytes has nothing to run
    if (!abIsPartialScript && GetHeader()->dataSize <= 4) {
        return false;
    }
    if (GetHeader()->bIsQuestScript && fQuestScriptDelay > 0.0) {
        fQuestScriptDelay = fQuestScriptDelay - appTimer.GetSeconds();
        fQuestScriptGetSecondsBuffer = appTimer.GetSeconds() + fQuestScriptGetSecondsBuffer;
        if (fQuestScriptDelay > 0.0) {
            return false;
        }
    }
    bool bResult = ScriptRunManager::Instance()->Run(
        this, ownerObject, ownerVars, ownerContainer, abIsPartialScript, false, false, 0.0f
    );
    if (GetHeader()->bIsQuestScript && fQuestScriptDelayTime.Float() > 0.0) {
        if (pOwnerQuest != nullptr && pOwnerQuest->GetScriptProcessingDelay() > 0.0) {
            fQuestScriptDelay = pOwnerQuest->GetScriptProcessingDelay() + fQuestScriptDelay;
        }
        else {
            fQuestScriptDelay = fQuestScriptDelay + fQuestScriptDelayTime.Float();
        }
        fQuestScriptGetSecondsBuffer = 0.0f;
    }
    ClearLocalVariableRefObjects();
    return bResult;
}

void Script::CompileAndRun(ScriptCompiler* compiler, COMPILER_NAME compilerIndex, TESObjectREFR* ownerObject) {
    alignas(AutoMemContext) char tempMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pTempMemContext = ::new (tempMemContext) AutoMemContext(
        MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESScript.cpp", 3411
    );
    if (compiler->CompilePartialScript(this, ownerObject, compilerIndex, false) && m_header.dataSize != 0) {
        if (compilerIndex == SYSTEM_WINDOW_COMPILER) {
            GetThreadSystemOutput() = true;
        }
        ScriptLocals* pScriptLocals = nullptr;
        if (ownerObject != nullptr) {
            pScriptLocals = ownerObject->GetScriptVariables();
        }
        Run(ownerObject, pScriptLocals, nullptr, true);
        GetThreadSystemOutput() = false;
    }
    pTempMemContext->~AutoMemContext();
}

void Script::RunScriptEffectStart(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals) {
    ScriptRunManager* singleton = ScriptRunManager::Instance();
    singleton->Run(this, apTargetRef, apEffectLocals, nullptr, false, true, false, 0.0);
}

void Script::RunScriptEffectFinish(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals) {
    ScriptRunManager* singleton = ScriptRunManager::Instance();
    singleton->Run(this, apTargetRef, apEffectLocals, nullptr, false, false, true, 0.0);
}

void Script::RunScriptEffectUpdate(TESObjectREFR* apTargetRef, ScriptLocals* apEffectLocals, float afSecondsElapsed) {
    ScriptRunManager* singleton = ScriptRunManager::Instance();
    singleton->Run(this, apTargetRef, apEffectLocals, nullptr, false, false, false, afSecondsElapsed);
}

void Script::SaveResultScript() {
    if (TESForm::QEndianSwapOnSave()) {
        (&m_header)->Endian();
    }
    TESForm::__AddChunkData(static_cast<CHUNK_ID>(1380467539), &m_header, 0x14u);
    if (TESForm::QEndianSwapOnSave()) {
        (&m_header)->Endian();
    }
}

bool Script::CompareResultScripts(Script* apCompare) {
    if (!apCompare) {
        return true;
    }
    SCRIPT_HEADER* compiledHeader = &apCompare->m_header;
    if (m_header.dataSize != compiledHeader->dataSize) {
        return true;
    }
    if (m_header.refObjectCount != compiledHeader->refObjectCount) {
        return true;
    }
    if (m_header.bIsCompiled != compiledHeader->bIsCompiled) {
        return true;
    }
    const char* compiledScript = apCompare->GetCompiledData();
    for (uint32_t n = 0; n < m_header.dataSize; ++n) {
        if (m_data[n] != compiledScript[n]) {
            return true;
        }
    }
    if (m_text && apCompare->m_text) {
        size_t v3 = std::strlen(m_text);
        if (v3 != std::strlen(apCompare->m_text)) {
            return true;
        }
        if (_tcscmp(m_text, apCompare->m_text)) {
            return true;
        }
    }
    else {
        if (m_text || apCompare->m_text) {
            return true;
        }
    }
    BSSimpleList<RefVariable*>* pNewList = &apCompare->listRefObjects;
    for (BSSimpleList<RefVariable*>* pOldList = &listRefObjects; pNewList && pOldList; pOldList = pOldList->m_pkNext) {
        RefVariable* pold = pOldList->m_item;
        RefVariable* pnew = pNewList->m_item;
        if (pnew && (pold && pnew->form != pold->form)) {
            return true;
        }
        pNewList = pNewList->m_pkNext;
    }
    return false;
}

bool Script::GetValue(
    double& adValue,
    const char* apCompiledData,
    uint32_t& auiDataOffset,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    bool abEndian
) {
    // the locals a variable operand reads: the script's own, or those of the referenced quest or reference
    ScriptLocals* pScriptLocals = apScriptLocals;
    if (pScriptLocals == nullptr) {
        return false;
    }
    RefVariable* pRefObject = nullptr;
    if (abEndian && apScript->GetInitialized()) {
        abEndian = false;
    }
    char cCode = apCompiledData[auiDataOffset++];
    switch (cCode) {
        // a referenced global, a referenced form's ID, or a variable or function of a referenced quest or reference
        case 'G':
        case 'Z':
        case 'r': {
            int16_t sIndex = *reinterpret_cast<const int16_t*>(&apCompiledData[auiDataOffset]);
            auiDataOffset += 2;
            pRefObject = apScript->GetReferencedObject(sIndex, apScriptLocals);
            if (pRefObject == nullptr) {
                return false;
            }
            if (pRefObject->form == nullptr && !abEndian) {
                // an unset reference variable reads as 0
                if (pRefObject->varIdx != 0) {
                    adValue = 0.0;
                    return true;
                }
                return false;
            }
            if (cCode == 'G') {
                adValue = static_cast<TESGlobal*>(pRefObject->form)->GetValue();
                return true;
            }
            if (cCode == 'Z') {
                uint32_t uiFormID = pRefObject->form->GetFormID();
                PutNumericIDInDouble(uiFormID, adValue);
                return true;
            }
            cCode = apCompiledData[auiDataOffset++];
            if (cCode == 'X') {
                break;
            }
            TESQuest* pQuest = nullptr;
            if (pRefObject->form != nullptr && pRefObject->form->GetFormType() == QUST_ID) {
                pQuest = static_cast<TESQuest*>(pRefObject->form);
            }
            if (pQuest != nullptr) {
                pScriptLocals = pQuest->GetScriptLocals();
            }
            else {
                TESObjectREFR* pRef = nullptr;
                if (pRefObject->form != nullptr) {
                    // the reference form types: REFR through PFLA, and PCBE
                    int32_t eType = pRefObject->form->GetFormType();
                    if (eType >= REFR_ID && (eType <= PFLA_ID || eType == PCBE_ID)) {
                        pRef = static_cast<TESObjectREFR*>(pRefObject->form);
                    }
                }
                if (pRef != nullptr) {
                    // an item inside a container keeps its locals in the container's inventory entry
                    if (pRef->GetExtra()->GetReferencePointer() != nullptr) {
                        TESObjectREFR* pContainer = pRef->GetExtra()->GetReferencePointer();
                        uint32_t uiFormID = pRef->GetFormID();
                        TESBoundObject* pObject = pRef->GetObjectReference();
                        ItemChange* pItem = pContainer->GetInventoryItem(pObject, uiFormID);
                        if (pItem != nullptr) {
                            pScriptLocals = pItem->GetScriptLocals();
                        }
                        if (pItem != nullptr) {
                            delete pItem;
                        }
                    }
                    else {
                        pScriptLocals = pRef->GetScriptVariables();
                    }
                }
            }
            if (pScriptLocals == nullptr) {
                return false;
            }
            break;
        }
        // an integer constant
        case 'n':
            adValue = *reinterpret_cast<const int32_t*>(&apCompiledData[auiDataOffset]);
            auiDataOffset += 4;
            return true;
        // a double constant
        case 'z':
            ourMemcpy(&adValue, &apCompiledData[auiDataOffset], sizeof(adValue));
            auiDataOffset += 8;
            return true;
        default: break;
    }
    if (cCode == 'X') {
        // a function call: the function index, then a 2-byte length the call skips
        int32_t iFunction = *reinterpret_cast<const int16_t*>(&apCompiledData[auiDataOffset]);
        auiDataOffset += 2;
        auiDataOffset += 2;
        SCRIPT_FUNCTION* pFunction = ScriptCompiler::GetFunctionDef(static_cast<SCRIPT_OUTPUT>(iFunction));
        if (pFunction == nullptr) {
            return false;
        }
        TESObjectREFR* pThisObj = apRefObject;
        if (pRefObject != nullptr && pRefObject->form != nullptr) {
            pThisObj = nullptr;
            if (pRefObject->form != nullptr) {
                // the reference form types: REFR through PFLA, and PCBE
                int32_t eType = pRefObject->form->GetFormType();
                if (eType >= REFR_ID && (eType <= PFLA_ID || eType == PCBE_ID)) {
                    pThisObj = static_cast<TESObjectREFR*>(pRefObject->form);
                }
            }
        }
        SCRIPT_PARAMETER* pParameters = pFunction->pParameters;
        if (abEndian) {
            if (pParameters != nullptr) {
                ParseParameters(
                    pParameters, apCompiledData, auiDataOffset, pThisObj, apContainer, apScript, apScriptLocals
                );
            }
            return false;
        }
        if (pFunction->bReferenceFunction && pThisObj == nullptr) {
            return false;
        }
        auto pExecuteFunction = pFunction->pExecuteFunction;
        return pExecuteFunction != nullptr &&
            pExecuteFunction(
                   pParameters, apCompiledData, pThisObj, apContainer, apScript, apScriptLocals, adValue, auiDataOffset
            );
    }
    // a local variable: 'f' float, 's' short, 'l' long
    int16_t sVariable = *reinterpret_cast<const int16_t*>(&apCompiledData[auiDataOffset]);
    auiDataOffset += 2;
    if (cCode == 'f' || cCode == 'l' || cCode == 's') {
        adValue = pScriptLocals->GetVariable(sVariable, apScript);
        return true;
    }
    return false;
}

void Script::GetNumericIDFromDouble(uint32_t& aiID, const double& adVal) {
    ourMemcpy(&aiID, &adVal, sizeof(aiID));
}

bool Script::IsPendingEnabledReference(TESObjectREFR* apRef) {
    bool v2 = false;
    Script::ScriptRefListCrit.Enter(nullptr);
    if (apRef) {
        v2 = Script::ReferencesToEnable.IsInList(apRef);
    }
    Script::ScriptRefListCrit.Leave();
    return v2;
}

bool Script::IsPendingFadeDisabledReference(TESObjectREFR* apRef) {
    bool v2 = false;
    Script::ScriptRefListCrit.Enter(nullptr);
    if (apRef) {
        v2 = Script::ReferencesToFadeDisable.IsInList(apRef);
    }
    Script::ScriptRefListCrit.Leave();
    return v2;
}

// The compiled data is untyped bytes: the parameter count, string lengths, reference indices and short values are read
// as unaligned 16-bit values. Each parsed parameter fills the pointer the next trailing argument holds. A referenced
// object is checked for its form type before the argument is taken; the spell, magic item and magic effect slots get
// the raw dynamic cast result, so they are untyped here. An integer parameter is converted by truncation.
bool Script::ParseParameters(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    uint32_t& auiOffset,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    ...
) {
    if (!apParameters) {
        return false;
    }
    va_list argList;
    va_start(argList, apScriptLocals);
    int16_t sCount = *reinterpret_cast<const int16_t*>(apCompiledParams + auiOffset);
    auiOffset += 2;
    for (int16_t i = 0; i < sCount; ++i) {
        if (scriptParameterDefs[apParameters[i].eParamType].bReferencedObject) {
            char cCode = apCompiledParams[auiOffset];
            ++auiOffset;
            if (cCode != 'r') {
                va_end(argList);
                return false;
            }
            int16_t sIndex = *reinterpret_cast<const int16_t*>(apCompiledParams + auiOffset);
            auiOffset += 2;
            RefVariable* pRef = apScript->GetReferencedObject(sIndex, apScriptLocals);
            if (!apScript->GetInitialized() || !pRef || !pRef->form) {
                va_end(argList);
                return false;
            }
            bool bFound = false;
            switch (apParameters[i].eParamType) {
                case SCRIPT_PARAM_INVENTORY_OBJECT:
                    if (TESContainer::ContainerCanHoldType(pRef->form->GetFormType())) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = nullptr;
                        if (pRef->form && pRef->form->IsObject()) {
                            *ppForm = pRef->form;
                        }
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_OBJECTREF:
                case SCRIPT_PARAM_MAP_MARKER:
                case SCRIPT_PARAM_CONTAINER_REF: {
                    uint8_t cType = pRef->form->GetFormType();
                    if (cType >= REFR_ID && (cType <= PFLA_ID || cType == PCBE_ID)) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = nullptr;
                        if (pRef->form) {
                            uint8_t cRefType = pRef->form->GetFormType();
                            if (cRefType >= REFR_ID && (cRefType <= PFLA_ID || cRefType == PCBE_ID)) {
                                *ppForm = pRef->form;
                            }
                        }
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                }
                case SCRIPT_PARAM_ACTOR: {
                    uint8_t cType = pRef->form->GetFormType();
                    if (cType >= ACHR_ID && cType <= ACRE_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                }
                case SCRIPT_PARAM_SPELL_ITEM: {
                    void** ppSpell = va_arg(argList, void**);
                    *ppSpell = dynamic_cast<SpellItem*>(pRef->form);
                    if (*ppSpell) {
                        bFound = true;
                    }
                    else {
                        // A book that teaches a spell stands in for it: the book's address is stored.
                        TESForm* pBook = nullptr;
                        if (pRef->form && pRef->form->GetFormType() == BOOK_ID) {
                            pBook = pRef->form;
                        }
                        *ppSpell = pBook;
                        if (pBook &&
                            static_cast<TESEnchantableForm*>(static_cast<TESObjectBOOK*>(pBook))->GetFormEnchanting()) {
                            bFound = true;
                        }
                    }
                    break;
                }
                case SCRIPT_PARAM_CELL:
                    if (pRef->form->GetFormType() == CELL_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_WORLD:
                    if (pRef->form->GetFormType() == WRLD_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_MAGIC_ITEM: {
                    void** ppItem = va_arg(argList, void**);
                    *ppItem = dynamic_cast<MagicItem*>(pRef->form);
                    if (*ppItem) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_MAGIC_EFFECT: {
                    void** ppEffect = va_arg(argList, void**);
                    *ppEffect = dynamic_cast<EffectSetting*>(pRef->form);
                    if (*ppEffect) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_SOUND:
                    if (pRef->form->GetFormType() == SOUN_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_MUSIC:
                    if (pRef->form->GetFormType() == MUSC_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_TOPIC:
                    if (pRef->form->GetFormType() == DIAL_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_QUEST:
                    if (pRef->form->GetFormType() == QUST_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_RACE:
                    if (pRef->form->GetFormType() == RACE_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_ENCOUNTERZONE:
                    if (pRef->form->GetFormType() == ECZN_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CLASS:
                    if (pRef->form->GetFormType() == CLAS_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_FACTION:
                    if (pRef->form->GetFormType() == FACT_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_REPUTATION:
                    if (pRef->form->GetFormType() == REPU_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CHALLENGE:
                    if (pRef->form->GetFormType() == CHAL_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CASINO:
                    if (pRef->form->GetFormType() == CSNO_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CASINOCHIP:
                    if (pRef->form->GetFormType() == CHIP_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CARAVANMONEY:
                    if (pRef->form->GetFormType() == CMNY_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CARAVANCARD:
                    if (pRef->form->GetFormType() == CCRD_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_CARAVANDECK:
                    if (pRef->form->GetFormType() == CDCK_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_REGION:
                    if (pRef->form->GetFormType() == REGN_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_GLOBAL:
                    if (pRef->form->GetFormType() == GLOB_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_FURNITURE_OR_FORMLIST:
                    if (pRef->form->GetFormType() == FURN_ID || pRef->form->GetFormType() == FLST_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_OBJECT: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form && pRef->form->IsObject()) {
                        *ppForm = pRef->form;
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_ACTOR_BASE: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form) {
                        uint8_t cType = pRef->form->GetFormType();
                        if (cType >= NPC__ID && cType <= CREA_ID) {
                            *ppForm = pRef->form;
                        }
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_PACKAGE:
                    if (pRef->form->GetFormType() == PACK_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_COMBAT_STYLE:
                    if (pRef->form->GetFormType() == CSTY_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_WEATHER:
                    if (pRef->form->GetFormType() == WTHR_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_IMAGESPACEMOD:
                    if (pRef->form->GetFormType() == IMAD_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_IMAGESPACE:
                    if (pRef->form->GetFormType() == IMGS_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_NPC:
                    if (pRef->form->GetFormType() == NPC__ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_OWNER: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form) {
                        uint8_t cType = pRef->form->GetFormType();
                        if (cType == FACT_ID) {
                            *ppForm = pRef->form;
                        }
                        else if (cType == NPC__ID) {
                            *ppForm = pRef->form;
                        }
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_SHADER_EFFECT:
                    if (pRef->form->GetFormType() == EFSH_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_FORMLIST:
                    if (pRef->form->GetFormType() == FLST_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_MENUICON:
                    if (pRef->form->GetFormType() == MICN_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_PERK:
                    if (pRef->form->GetFormType() == PERK_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_NOTE:
                    if (pRef->form->GetFormType() == NOTE_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_MESSAGE:
                    if (pRef->form->GetFormType() == MESG_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_INVOBJECT_OR_FORMLIST:
                    if (TESContainer::ContainerCanHoldType(pRef->form->GetFormType()) ||
                        pRef->form->GetFormType() == FLST_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_OBJECT_OR_FORMLIST: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form && (pRef->form->IsObject() || pRef->form->GetFormType() == FLST_ID)) {
                        *ppForm = pRef->form;
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_NPC_OR_LEVCHAR: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form) {
                        uint8_t cType = pRef->form->GetFormType();
                        if (cType == NPC__ID) {
                            *ppForm = pRef->form;
                        }
                        else if (cType == LVLN_ID) {
                            *ppForm = pRef->form;
                        }
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_CREA_OR_LEVCREA: {
                    TESForm** ppForm = va_arg(argList, TESForm**);
                    *ppForm = nullptr;
                    if (pRef->form) {
                        uint8_t cType = pRef->form->GetFormType();
                        if (cType == CREA_ID) {
                            *ppForm = pRef->form;
                        }
                        else if (cType == LVLC_ID) {
                            *ppForm = pRef->form;
                        }
                    }
                    if (*ppForm) {
                        bFound = true;
                    }
                    break;
                }
                case SCRIPT_PARAM_LEVCHAR:
                    if (pRef->form->GetFormType() == LVLN_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_LEVCREA:
                    if (pRef->form->GetFormType() == LVLC_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_LEVITEM:
                    if (pRef->form->GetFormType() == LVLI_ID) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                case SCRIPT_PARAM_FORM:
                    if (pRef->form) {
                        TESForm** ppForm = va_arg(argList, TESForm**);
                        *ppForm = pRef->form;
                        if (*ppForm) {
                            bFound = true;
                        }
                    }
                    break;
                default:
                    Warning(
                        "DEFAULT: Param type '%d' (referenced object) unimplemented in Script::ParseParameters.",
                        apParameters[i].eParamType
                    );
                    va_end(argList);
                    return false;
            }
            if (!bFound) {
                va_end(argList);
                return false;
            }
            continue;
        }

        switch (apParameters[i].eParamType) {
            case SCRIPT_PARAM_CHAR: {
                int16_t sLength = *reinterpret_cast<const int16_t*>(apCompiledParams + auiOffset);
                auiOffset += 2;
                if (apScript->GetInitialized()) {
                    char* pString = va_arg(argList, char*);
                    ourMemcpy(pString, apCompiledParams + auiOffset, sLength);
                    pString[sLength] = 0;
                }
                auiOffset += sLength;
                break;
            }
            case SCRIPT_PARAM_INT:
            case SCRIPT_PARAM_STAGE: {
                double dValue = 0.0;
                if (!GetValue(
                        dValue, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, true
                    )) {
                    va_end(argList);
                    return false;
                }
                if (apScript->GetInitialized()) {
                    int32_t* piValue = va_arg(argList, int32_t*);
                    *piValue = static_cast<int32_t>(dValue);
                }
                break;
            }
            case SCRIPT_PARAM_FLOAT: {
                double dValue = 0.0;
                if (!GetValue(
                        dValue, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, true
                    )) {
                    va_end(argList);
                    return false;
                }
                if (apScript->GetInitialized()) {
                    float* pfValue = va_arg(argList, float*);
                    *pfValue = static_cast<float>(dValue);
                }
                break;
            }
            case SCRIPT_PARAM_AXIS:
                if (apScript->GetInitialized()) {
                    char* pcAxis = va_arg(argList, char*);
                    *pcAxis = apCompiledParams[auiOffset];
                }
                ++auiOffset;
                break;
            case SCRIPT_PARAM_ACTOR_VALUE:
            case SCRIPT_PARAM_MISC_STAT:
            case SCRIPT_PARAM_ALIGNMENT:
            case SCRIPT_PARAM_EQUIPTYPE:
            case SCRIPT_PARAM_CRITSTAGE:
            case SCRIPT_PARAM_CRIME_TYPE:
            case SCRIPT_PARAM_ANIM_GROUP:
            case SCRIPT_PARAM_SEX: {
                int16_t sValue = *reinterpret_cast<const int16_t*>(apCompiledParams + auiOffset);
                auiOffset += 2;
                if (apScript->GetInitialized()) {
                    int32_t* piValue = va_arg(argList, int32_t*);
                    *piValue = sValue;
                }
                break;
            }
            case SCRIPT_PARAM_FORM_TYPE: {
                int16_t sValue = *reinterpret_cast<const int16_t*>(apCompiledParams + auiOffset);
                auiOffset += 2;
                if (apScript->GetInitialized()) {
                    uint8_t* pcValue = va_arg(argList, uint8_t*);
                    *pcValue = static_cast<uint8_t>(sValue);
                }
                break;
            }
            default: va_end(argList); return false;
        }
    }
    va_end(argList);
    return true;
}

void Script::CopyReferencedObjectList(
    BSSimpleList<RefVariable*>* pCopyFromList,
    BSSimpleList<RefVariable*>* pCopyToList,
    Script*
) {
    if (pCopyToList == nullptr || pCopyFromList == nullptr) {
        return;
    }
    while (pCopyFromList != nullptr && !pCopyFromList->IsEmpty()) {
        void* pMemory = ourMalloc(sizeof(RefVariable));
        RefVariable* pCopy = pMemory ? ::new (pMemory) RefVariable(pCopyFromList->GetItem()) : nullptr;
        pCopyToList->AddTail(pCopy);
        pCopyFromList = pCopyFromList->GetNext();
    }
}

void Script::CopyVariableList(
    BSSimpleList<ScriptVariable*>* pCopyFromList,
    BSSimpleList<ScriptVariable*>* pCopyToList,
    Script*
) {
    if (pCopyToList == nullptr || pCopyFromList == nullptr) {
        return;
    }
    while (pCopyFromList != nullptr && !pCopyFromList->IsEmpty()) {
        void* pMemory = ourMalloc(sizeof(ScriptVariable));
        ScriptVariable* pCopy = pMemory ? ::new (pMemory) ScriptVariable(pCopyFromList->GetItem()) : nullptr;
        pCopyToList->AddTail(pCopy);
        pCopyFromList = pCopyFromList->GetNext();
    }
}

bool Script::IsPendingDisabledReference(TESObjectREFR* apRef) {
    bool bResult = false;
    ScriptRefListCrit.Enter(nullptr);
    if (apRef != nullptr && ReferencesToDisable.IsInList(apRef)) {
        bResult = true;
    }
    ScriptRefListCrit.Leave();
    return bResult;
}

void Script::ClearOptimizations() {
    GetThreadLastRefSearchScript() = nullptr;
    GetThreadLastScriptLocals() = nullptr;
    GetThreadLastRefSearchIndex() = 0xFFFFFFFF;
    GetThreadLastRefObject() = nullptr;
}

void Script::SetProcessScripts(bool abProcessScripts) {
    bIsScriptProcessing = abProcessScripts;
}

void Script::RunDelayedScriptActionsOnReferences() {
    ScriptRefListCrit.Enter(nullptr);
    while (!ReferencesToEnable.IsEmpty()) {
        TESObjectREFR* pRef = ReferencesToEnable.GetItem();
        ReferencesToEnable.RemoveHead();
        pRef->Enable();
    }
    while (!ReferencesToDisable.IsEmpty()) {
        TESObjectREFR* pRef = ReferencesToDisable.GetItem();
        ReferencesToDisable.RemoveHead();
        pRef->Disable();
    }
    // a reference waits in the fade list until its fade node has faded out
    BSSimpleList<TESObjectREFR*>* pNode = &ReferencesToFadeDisable;
    while (pNode != nullptr && !pNode->IsEmpty()) {
        BSSimpleList<TESObjectREFR*>* pNext = pNode->GetNext();
        TESObjectREFR* pRef = pNode->GetItem();
        NiNode* p3D = pRef->Get3D();
        BSFadeNode* pFadeNode = p3D != nullptr ? p3D->IsFadeNode() : nullptr;
        if (pFadeNode == nullptr || !pFadeNode->IsVisible()) {
            pRef->Disable();
            ReferencesToFadeDisable.Remove(pRef);
            // removing the head moves the next item into the head node
            if (pNode == &ReferencesToFadeDisable) {
                pNext = &ReferencesToFadeDisable;
            }
        }
        pNode = pNext;
    }
    while (!ReferencesToDelete.IsEmpty()) {
        TESObjectREFR* pRef = ReferencesToDelete.GetItem();
        ReferencesToDelete.RemoveHead();
        if (pRef != nullptr) {
            delete pRef;
        }
    }
    ScriptRefListCrit.Leave();
}

void Script::RemoveDelayedScriptActionReference(TESObjectREFR* apRef) {
    if (apRef) {
        (&Script::ScriptRefListCrit)->Enter(nullptr);
        (&Script::ReferencesToEnable)->Remove(apRef);
        (&Script::ReferencesToDisable)->Remove(apRef);
        (&Script::ReferencesToFadeDisable)->Remove(apRef);
        (&Script::ReferencesToDelete)->Remove(apRef);
        (&Script::ScriptRefListCrit)->Leave();
    }
}

void Script::AddPendingEnabledReference(TESObjectREFR* apRef) {
    (&Script::ScriptRefListCrit)->Enter(nullptr);
    if (apRef) {
        (&Script::ReferencesToDisable)->Remove(apRef);
        if (!Script::IsPendingEnabledReference(apRef)) {
            (&Script::ReferencesToEnable)->AddHead(apRef);
        }
    }
    (&Script::ScriptRefListCrit)->Leave();
}

float TESQuest::GetScriptProcessingDelay() {
    return data.fQuestDelayTime;
}

char* TESSaveLoadGame::GetMostRecentSaveGame() {
    return m_pMostRecentSaveGame;
}

void ScriptVariable::operator delete(void* apMemory) {
    ourFree(apMemory);
}
