#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/IOManager.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormIDMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadInitialData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadReferencesMap.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

SaveGameFormInfo::SaveGameFormInfo()
  : iFormIDMapOffset(0), iHistoryOffset(0), iInitialGlobalDataOffset(0), iFormDataOffset(0), iFinalGlobalDataOffset(0),
    iInitialGlobalFormCount(0), iFinalGlobalFormCount(0), iFormCount(0), iSecondaryDataSize(0), ReservedSpace{} {}

BGSSaveLoadGame::BGSSaveLoadGame()
  : pOldChangesMap(nullptr), FormBufferArray(), QueuedInitPackageLocationsActorMap(37u), uiGlobalFlags(0),
    cCurrentMinorVersion(0xFF) {
    SetGlobalAllowChanges(false);
    pChangesMap = new BGSSaveLoadChangesMap();
    pFormIDMap = new BGSSaveLoadFormIDMap(0x13AFu);
    pWorldspaceFormIDMap = new BGSSaveLoadFormIDMap(37u);
    pReferencesMap = new BGSSaveLoadReferencesMap();
    pQueuedSubBuffersMap = new BGSSaveLoadQueuedSubBufferMap();
    pChangedFormIDMap = new NiTMap<uint32_t, uint32_t>(37u);
    pHistory = new BGSSaveLoadHistory();
    pReconstructForms = new BGSReconstructFormsInAllFilesMap();
    ourMemset(cFileIndexArray, 255, 255u);
    ourMemset(cReverseFileIndexArray, 255, 255u);
}

BGSSaveLoadGame::~BGSSaveLoadGame() {
    delete pChangesMap;
    delete pFormIDMap;
    delete pWorldspaceFormIDMap;
    delete pReferencesMap;
    delete pQueuedSubBuffersMap;
    delete pChangedFormIDMap;
    delete pHistory;
    delete pReconstructForms;
}

void BGSSaveLoadGame::QueueInitPackageLocations(Actor* apActor) {
    ActorValueOwner* pActorValueOwner = static_cast<ActorValueOwner*>(apActor->GetActorBase());
    if (pActorValueOwner->GetActorFloatValue(ActorValue::HEALTH) > 0.0) {
        QueuedInitPackageLocationsActorMap.SetAt(apActor->iFormID, apActor);
    }
}

void BGSSaveLoadGame::DeleteForm(TESForm* apForm) {
    if (apForm) {
        delete apForm;
    }
}

void BGSSaveLoadGame::CancelInitPackageLocations(TESObjectREFR* apRef) {
    QueuedInitPackageLocationsActorMap.RemoveAt(apRef->iFormID);
}

void BGSSaveLoadGame::AddChange(TESForm* apForm, BGSChangeFlags aiChanges, bool abForceAdd) {
    if (!ShouldAddChanges(apForm, abForceAdd)) {
        return;
    }
    pChangesMap->AddChangeFlags(apForm->iFormID, aiChanges);
}

void BGSSaveLoadGame::RemoveChange(TESForm* apForm, BGSChangeFlags aiChanges) {
    if (!ShouldRemoveChanges(apForm)) {
        return;
    }
    pChangesMap->RemoveChangeFlags(apForm->iFormID, aiChanges);
}

// Drops the form's changes unless they still hold an unloaded form buffer.
void BGSSaveLoadGame::RemoveChanges(TESForm* apForm) {
    uint32_t refID = apForm->GetFormID();
    BGSFormChanges* pChanges = pChangesMap->GetChanges(refID);
    if (pChanges && !static_cast<char*>(pChanges->UnloadedFormBuffer)) {
        pChangesMap->RemoveChanges(refID);
    }
}

bool BGSSaveLoadGame::GetOldChange(TESForm* apForm, BGSChangeFlags aiChanges) {
    if (!pOldChangesMap) {
        return false;
    }
    return pOldChangesMap->CheckChangeFlags(apForm->GetFormID(), aiChanges);
}

void BGSSaveLoadGame::QueueSubBuffer(
    QUEUED_SUB_BUFFER_TYPE aeType,
    BGSLoadFormBuffer* apLoadFormBuffer,
    TESForm* apParentForm
) {
    pQueuedSubBuffersMap->QueueSubBuffer(aeType, apLoadFormBuffer, apParentForm);
}

void BGSSaveLoadGame::RevertAllChanges() {
    SetSaveGameLoading(true);
    BGSSaveLoadGlobalData::RevertGlobalData(true);
    RevertCurrentChanges(nullptr, nullptr);
    PerformQueuedInitPackageLocations();
    SetSaveGameLoading(false);
}

void BGSSaveLoadGame::AddChangedFormID(uint32_t aiOldFormID, uint32_t aiNewFormID) {
    uint32_t uiCurrentFormID = 0;
    if (pChangedFormIDMap->GetAt(aiOldFormID, uiCurrentFormID)) {
        SaveGameWarning(
            "FORMS: Trying to set new form ID %08X for old form ID %08X, but already in map with value "
            "%08X",
            aiNewFormID,
            aiOldFormID,
            uiCurrentFormID
        );
    }
    pChangedFormIDMap->SetAt(aiOldFormID, aiNewFormID);
}

bool BGSSaveLoadGame::ShouldAddChanges(TESForm* apForm, bool abForceAdd) {
    if (!GetAllowChanges() && !abForceAdd) {
        return false;
    }
    if (apForm->GetTemporary()) {
        return false;
    }
    uint32_t iFormID = apForm->iFormID;
    return !(apForm->GetDelete() && pDataHandler->IsFormIDCreated(iFormID));
}

bool BGSSaveLoadGame::GetChange(TESForm* apForm, BGSChangeFlags aiChanges) {
    return pChangesMap->CheckChangeFlags(apForm->iFormID, aiChanges);
}

bool BGSSaveLoadGame::IsFormIDInUse(uint32_t aiFormID) {
    return pChangesMap->GetChanges(aiFormID) != nullptr;
}

bool BGSSaveLoadGame::HasReferenceChangedCells(uint32_t aiFormID) {
    return pReferencesMap->GetMovedReference(aiFormID);
}

bool BGSSaveLoadGame::GetSaveGameLoading() const {
    return (uiGlobalFlags & GLOBAL_SAVEGAME_LOADING) != 0;
}

bool BGSSaveLoadGame::GetSaveGameSaving() const {
    return (uiGlobalFlags & GLOBAL_SAVEGAME_SAVING) != 0;
}

bool BGSSaveLoadGame::GetDeferInitForms() const {
    return (uiGlobalFlags & GLOBAL_DEFER_INIT_FORMS) != 0;
}

bool BGSSaveLoadGame::GetGlobalAllowChanges() const {
    return (uiGlobalFlags & GLOBAL_DISALLOW_CHANGES) == 0;
}

bool BGSSaveLoadGame::GetInitingForms() const {
    return (uiGlobalFlags & GLOBAL_INITING_FORMS) != 0;
}

void BGSSaveLoadGame::SetInitingForms(bool abInitingForms) {
    if (abInitingForms) {
        uiGlobalFlags |= GLOBAL_INITING_FORMS;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_INITING_FORMS);
    }
}

void BGSSaveLoadGame::SetSaveLoadFailed(bool abFailed) {
    if (abFailed) {
        uiGlobalFlags |= GLOBAL_SAVELOAD_FAILED;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_SAVELOAD_FAILED);
    }
}

bool BGSSaveLoadGame::SetGlobalAllowChanges(bool abValue) {
    bool bWasAllowed = GetGlobalAllowChanges();
    if (!abValue) {
        uiGlobalFlags |= GLOBAL_DISALLOW_CHANGES;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_DISALLOW_CHANGES);
    }
    return bWasAllowed;
}

bool BGSSaveLoadGame::GetAllowChanges() {
    return GetThreadAllowChanges() && GetGlobalAllowChanges();
}

// Changes are tracked only while changes are allowed, and never for temporary forms.
bool BGSSaveLoadGame::ShouldRemoveChanges(TESForm* apForm) {
    return GetAllowChanges() && !apForm->GetTemporary();
}

void BGSSaveLoadGame::SetLoadingMovedRefs(bool abValue) {
    if (abValue) {
        uiThreadSpecificFlags |= THREAD_FLAG_LOADING_MOVED_REFS;
    }
    else {
        uiThreadSpecificFlags &= ~THREAD_FLAG_LOADING_MOVED_REFS;
    }
}

void BGSSaveLoadGame::UnloadForm(TESForm* apForm, bool abUnloadWhileLoadingSaveGame) {
    BGSChangeFlags iremovedFlags;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_SAVELOAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\SaveLoad\\BGSSaveLoadGame.cpp",
        894
    );
    if ((GetSaveGameLoading() && !abUnloadWhileLoadingSaveGame) ||
        (pDataHandler->GetClearingData() || !ShouldUnloadForm(apForm))) {
        return;
    }
    uint32_t iformID = apForm->iFormID;
    BGSFormChanges* pchanges = pChangesMap->GetChanges(iformID);
    if (pchanges) {
        if (pchanges->UnloadedFormBuffer.pBuffer) {
            SaveGameWarning(
                "FORMS: Form '%s' (%08X) is unloading, but it already has a buffer.  This unload will be "
                "skipped.",
                apForm->GetFormEditorID(),
                iformID
            );
            return;
        }
        BGSSaveFormBuffer saveFormBuffer;
        uint8_t ucMinorVersion = pSaveLoadManager->GetMinorVersion();
        ENUM_FORM_ID eFormType = apForm->GetFormType();
        saveFormBuffer.SetHeader(iformID, pchanges->iChangeFlags, eFormType, ucMinorVersion);
        saveFormBuffer.pForm = apForm;
        apForm->CheckSaveGame(&saveFormBuffer);
        int32_t iBufferFlags = saveFormBuffer.GetChangeFlags().iFlags;
        pchanges->iChangeFlags.GetDifference(&iremovedFlags, iBufferFlags);
        if (iremovedFlags.iFlags && pChangesMap->RemoveChangeFlags(iformID, iremovedFlags)) {
            return;
        }
        INITIAL_DATA_TYPE einitialDataType = BGSSaveLoadInitialData::SaveInitialData(
            &saveFormBuffer,
            apForm,
            pchanges->iChangeFlags
        );
        apForm->SaveGame(&saveFormBuffer);
        pchanges->UnloadedFormBuffer.Initialize(&saveFormBuffer);
        if (einitialDataType == INITIAL_DATA_CREATED_REFERENCE || einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
            TESObjectREFR* pref = static_cast<TESObjectREFR*>(apForm);
            TESObjectCELL* pcell = pref->GetParentCell();
            TESWorldSpace* pworld = pref->GetWorldSpace();
            if (pcell && pcell->GetInterior()) {
                uint32_t uiCellID = pcell->iFormID;
                pReferencesMap->AddReference(uiCellID, iformID);
                if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
                    pReferencesMap->AddMovedReference(iformID);
                }
            }
            else if (pworld) {
                const NiPoint3& kLocation = pref->GetLocationOnReference();
                uint32_t uiWorldID = pworld->iFormID;
                pReferencesMap->AddReference(uiWorldID, iformID, kLocation);
                if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
                    pReferencesMap->AddMovedReference(iformID);
                }
            }
            else {
                SaveGameWarning(
                    "FORMS: No Worldspace or Cell could not be found while unloading reference '%s' "
                    "(%08X)",
                    pref->GetFormEditorID(),
                    iformID
                );
            }
        }
    }
}

void BGSSaveLoadGame::LoadForm(TESForm* apForm) {
    if (!GetSaveGameLoading()) {
        uint32_t iformID = apForm->iFormID;
        BGSFormChanges* pchanges = pChangesMap->GetChanges(iformID);
        if (pchanges && pchanges->UnloadedFormBuffer.pBuffer) {
            ENUM_FORM_ID eformType = pchanges->UnloadedFormBuffer.GetFormType();
            if (apForm->GetFormType() == eformType) {
                bool brestoreAllowChanges = SetThreadAllowChanges(false);
                BGSLoadFormBuffer* ploadFormBuffer = pchanges->UnloadedFormBuffer.FillLoadFormBuffer();
                // The load buffer owns the data: DestructKeepBuffer only clears the pointer.
                BGSUnloadedFormBuffer::DestructKeepBuffer(&pchanges->UnloadedFormBuffer);
                ploadFormBuffer->pForm = apForm;
                ploadFormBuffer->SetChangeFlags(pchanges->iChangeFlags);
                apForm->SetStillLoading(true);
                BGSSaveLoadInitialData::LoadInitialData(ploadFormBuffer, apForm, pchanges->iChangeFlags);
                apForm->LoadGame(ploadFormBuffer);
                ploadFormBuffer->ClearBuffer();
                FormBufferArray.Add(ploadFormBuffer);
                SetThreadAllowChanges(brestoreAllowChanges);
            }
            else {
                const char* pcFormTypeString = apForm->GetFormTypeString();
                const char* pcSavedFormTypeString = TESForm::GetFormTypeString(eformType);
                SaveGameWarning(
                    "SAVELOAD: (Load Error) Form '%s' (%08X) was saved with form type %s, but "
                    "currently has form type %s.  Its loading will be skipped.",
                    apForm->GetFormEditorID(),
                    iformID,
                    pcSavedFormTypeString,
                    pcFormTypeString
                );
                pChangesMap->RemoveChanges(iformID);
            }
        }
    }
}

void BGSSaveLoadGame::SaveGame(BGSSaveLoadFile* apFile) {
    SetSaveGameSaving(true);
    char acFullFileName[0x108];
    pSaveLoadManager->GetFullFileName(apFile->GetSaveFileName(), acFullFileName);
    char acVersionInfo[0x104];
    pSaveLoadManager->GetVersionInfo(acVersionInfo, 0x104);
    pHistory->AddNote("***Saving game: %s (%s)", acFullFileName, acVersionInfo);
    uint8_t ucMinorVersion = static_cast<uint8_t>(pSaveLoadManager->GetMinorVersion());
    SaveGameDataToFileFunction(apFile, &ucMinorVersion, 1);
    SavePluginList(apFile);
    uint32_t uiHeaderPosition = apFile->GetFilePosition();
    SaveGameFormInfo kHeader;
    apFile->Write(&kHeader, 0x6E);
    kHeader.iInitialGlobalDataOffset = apFile->GetFilePosition();
    kHeader.iInitialGlobalFormCount = BGSSaveLoadFile::SaveGlobalDataRange0_C(apFile);
    kHeader.iFormDataOffset = apFile->GetFilePosition();
    alignas(BGSSaveFormBuffer) uint8_t kBufferMemory[sizeof(BGSSaveFormBuffer)];
    BGSSaveFormBuffer* pBuffer = ::new (kBufferMemory) BGSSaveFormBuffer;
    NiTMapItem<uint32_t, BGSFormChanges*>* pPos = pChangesMap->GetFirstPos();
    while (pPos) {
        uint32_t uiFormID = 0;
        BGSFormChanges* pChanges = nullptr;
        pChangesMap->GetNext(pPos, uiFormID, pChanges);
        if (pChanges->UnloadedFormBuffer) {
            pChanges->UnloadedFormBuffer.Save(apFile, uiFormID, pChanges->iChangeFlags);
            ++kHeader.iFormCount;
            continue;
        }
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        if (!pForm) {
            SaveGameWarning(
                "SAVELOAD: Could not find form %08X with change flags %08X during "
                "BGSSaveLoadGame::SaveGame()",
                uiFormID,
                static_cast<int32_t>(pChanges->iChangeFlags)
            );
            continue;
        }
        pBuffer->ClearPosition();
        // Evaluates the version, then the form type, then the flags.
        uint8_t ucVersion = static_cast<uint8_t>(pSaveLoadManager->GetMinorVersion());
        ENUM_FORM_ID eFormType = pForm->GetFormType();
        pBuffer->SetHeader(uiFormID, pChanges->iChangeFlags, eFormType, ucVersion);
        pBuffer->SetForm(pForm);
        pForm->CheckSaveGame(pBuffer);
        // The flags the buffer gained over the recorded ones are recorded.
        BGSChangeFlags kRecorded;
        kRecorded.iFlags = pChanges->iChangeFlags;
        BGSChangeFlags kAdded;
        pBuffer->GetChangeFlags().GetDifference(&kAdded, kRecorded.iFlags);
        if (kAdded) {
            pChangesMap->AddChangeFlags(uiFormID, kAdded);
        }
        // The recorded flags the buffer lost are removed; when that empties the entry, the form is not saved.
        BGSChangeFlags kBufferFlags;
        kBufferFlags.iFlags = pBuffer->GetChangeFlags();
        BGSChangeFlags kRemoved;
        pChanges->iChangeFlags.GetDifference(&kRemoved, kBufferFlags.iFlags);
        if (kRemoved && pChangesMap->RemoveChangeFlags(uiFormID, kRemoved)) {
            continue;
        }
        BGSSaveLoadInitialData::SaveInitialData(pBuffer, pForm, pChanges->iChangeFlags);
        pForm->SaveGame(pBuffer);
        pBuffer->Save(apFile);
        ++kHeader.iFormCount;
    }
    kHeader.iFinalGlobalDataOffset = apFile->GetFilePosition();
    kHeader.iFinalGlobalFormCount = BGSSaveLoadFile::SaveGlobalDataRange3E8_3E9(apFile);
    kHeader.iFormIDMapOffset = apFile->GetFilePosition();
    pFormIDMap->Save(apFile);
    pWorldspaceFormIDMap->Save(apFile);
    kHeader.iHistoryOffset = apFile->GetFilePosition();
    pHistory->AddNote("---Finished saving game: %s", acFullFileName);
    pHistory->Save(apFile);
    apFile->SeekSet(uiHeaderPosition);
    apFile->Write(&kHeader, 0x6E);
    SetSaveGameSaving(false);
    pBuffer->~BGSSaveFormBuffer();
}

void BGSSaveLoadGame::SavePluginList(BGSSaveLoadFile* apFile) {
    // Raw storage: only BGSSaveGameBuffer's own constructor runs on this stack buffer.
    alignas(BGSSaveGameBuffer) char aBufferStorage[sizeof(BGSSaveGameBuffer)];
    BGSSaveGameBuffer* pBuffer = ::new (aBufferStorage) BGSSaveGameBuffer;
    int8_t cCount = static_cast<int8_t>(pDataHandler->TESDataHandler::CountCompiled());
    pBuffer->SaveDataEndian(&cCount, sizeof(cCount), 0);
    for (uint32_t uiIndex = 0; uiIndex < static_cast<uint8_t>(cCount); ++uiIndex) {
        TESFile* pFile = pDataHandler->GetCompiledFile(uiIndex);
        const char* pcFilename = pFile->GetFilename();
        pBuffer->SaveString(pcFilename, 0);
    }
    pBuffer->Save(apFile);
    pBuffer->~BGSSaveGameBuffer();
}

// Reverts every changed form to its loaded state: created forms are cleared, the others revert through a revert-only
// load buffer (their unrevertible changes handled or the form queued for reconstruction), and an unloaded moved
// reference whose original cell is loaded is queued for reconstruction. apChangesMap (a new map when null) replaces the
// changes map, which is deleted.
void BGSSaveLoadGame::RevertCurrentChanges(
    BGSSaveLoadChangesMap* apNewChangesMap,
    BGSSaveLoadFormIDMap* apOldFormIDMap
) {
    if (!apNewChangesMap) {
        apNewChangesMap = ::new BGSSaveLoadChangesMap;
    }
    NiTMapItem<uint32_t, BGSFormChanges*>* pPos = pChangesMap->GetFirstPos();
    while (pPos) {
        uint32_t iformID = 0;
        BGSFormChanges* pchanges = nullptr;
        pChangesMap->GetNext(pPos, iformID, pchanges);
        TESForm* pform = TESForm::GetFormByNumericID(iformID);
        if (pform) {
            if (pDataHandler->IsFormIDCreated(iformID)) {
                ClearForm(pform);
                apNewChangesMap->RemoveChanges(iformID);
            }
            else {
                BGSLoadFormBuffer kLoadFormBuffer;
                kLoadFormBuffer.pForm = pform;
                kLoadFormBuffer.SetOldChangeFlags(pchanges->iChangeFlags);
                kLoadFormBuffer.SetRevertOnly(true);
                pform->Revert(&kLoadFormBuffer);
                uint32_t eFormType = pform->GetFormType();
                BGSChangeFlags kNoChanges(0);
                BGSChangeFlags kUnrevertible;
                BGSChangeFlags::GetUnrevertibleChanges(
                    &kUnrevertible,
                    pchanges->iChangeFlags,
                    kNoChanges,
                    eFormType,
                    false
                );
                if (kUnrevertible.iFlags && !HandleUnrevertibleChanges(pform, kUnrevertible, true) && pform) {
                    pReconstructForms->AddForm(pform, 8u);
                }

                if (pform && pchanges->iChangeFlags.ReduceToForcedChanges(pform)) {
                    apNewChangesMap->AddChangeFlags(iformID, pchanges->iChangeFlags);
                }
            }
        }
        else if (apOldFormIDMap) {
            BGSUnloadedFormBuffer unloadedFormBuffer;
            unloadedFormBuffer.pBuffer = pchanges->UnloadedFormBuffer.pBuffer;
            if (unloadedFormBuffer.pBuffer) {
                ENUM_FORM_ID eformType = unloadedFormBuffer.GetFormType();
                INITIAL_DATA_TYPE einitialDataType = BGSSaveLoadInitialData::GetInitialDataType(
                    iformID,
                    eformType,
                    pchanges->iChangeFlags
                );
                if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
                    BGSSaveLoadFormIDMap* pcurrentFormIDMap = pFormIDMap;
                    pFormIDMap = apOldFormIDMap;
                    BGSUnloadedFormBuffer changeBuffer;
                    pChangesMap->GetChangeBuffer(&changeBuffer, iformID);
                    ReferenceInitialData refData;
                    BGSSaveLoadInitialData::LoadInitialDataStruct(
                        changeBuffer.GetAdvancedBuffer(),
                        einitialDataType,
                        &refData
                    );
                    TESObjectCELL* pcell = nullptr;
                    TESWorldSpace* pworld = nullptr;
                    refData.GetOriginalLocationCellAndWorld(pcell, pworld);
                    if (pcell && TES::pInstance->IsCellLoaded(pcell, false)) {
                        pReconstructForms->AddReference(pcell, iformID, 8u);
                    }

                    pFormIDMap = pcurrentFormIDMap;
                }
            }
        }
    }
    pReconstructForms->ReconstructForms();
    pReconstructForms->Clear();
    delete pChangesMap;
    pChangesMap = apNewChangesMap;
}

void BGSSaveLoadGame::PerformQueuedInitPackageLocations() {
    NiTMapItem<uint32_t, Actor*>* it = QueuedInitPackageLocationsActorMap.GetFirstPos();
    while (it) {
        uint32_t iformID = 0;
        Actor* pactor = nullptr;
        QueuedInitPackageLocationsActorMap.GetNext(it, iformID, pactor);
        if (pactor->pCurrentProcess) {
            pactor->InitPackageLocations(true);
        }
    }
    QueuedInitPackageLocationsActorMap.RemoveAll();
}

void BGSSaveLoadGame::ClearForm(TESForm* apForm) {
    if (apForm) {
        uint32_t uiFormType = apForm->GetFormType();
        switch (uiFormType) {
            case 4u:
            case 17u:
            case 18u:
            case 42u:
            case 43u:
            case 57u:
            case 66u:
            case 67u: {
                uint32_t uiNextID = pDataHandler->GetNextID();
                apForm->SetFormID(uiNextID, true);
                break;
            }
            case 58u:
            case 59u:
            case 60u:
            case 61u:
            case 62u:
            case 63u:
            case 64u:
            case 73u:
            case 105u: {
                DeleteForm(apForm);
                break;
            }
            default: {
                const char* pcFormTypeString = TESForm::GetFormTypeString(uiFormType);
                int32_t iFormID = apForm->GetFormID();
                SaveGameWarning(
                    "FORMS: Form %08X with form type %i %s was re-numbered by BGSSaveLoadGame::ClearForm()",
                    iFormID,
                    uiFormType,
                    pcFormTypeString
                );
                uint32_t uiNextID = pDataHandler->GetNextID();
                apForm->SetFormID(uiNextID, true);
                break;
            }
        }
    }
}

// Outside a save load, loads the cell's saved changes, then its saved references (with moved references flagged),
// rebuilds the forms they need and loads each.
void BGSSaveLoadGame::LoadCell(TESObjectCELL* apCell) {
    if (GetSaveGameLoading()) {
        return;
    }
    LoadForm(apCell);
    alignas(BGSReconstructFormsInAllFilesMap) uint8_t aMapStorage[sizeof(BGSReconstructFormsInAllFilesMap)];
    BGSReconstructFormsInAllFilesMap& rReconstructForms = *::new (aMapStorage) BGSReconstructFormsInAllFilesMap;
    pReferencesMap->LoadReferencesForCell(apCell, &rReconstructForms);
    SetLoadingMovedRefs(true);
    rReconstructForms.ReconstructForms();
    SetLoadingMovedRefs(false);
    uint32_t uiCount = rReconstructForms.GetReconstructedCount();
    for (uint32_t i = 0; i < uiCount; ++i) {
        TESForm* pForm = rReconstructForms.GetReconstructedForm(i);
        LoadForm(pForm);
    }
    rReconstructForms.~BGSReconstructFormsInAllFilesMap();
}

void BGSSaveLoadGame::InitForms(bool abLoadModels) {
    SetInitingForms(true);
    if (abLoadModels) {
        pIOManager->ForceHighestPriority();
    }
    for (uint32_t i = 0; i < this->FormBufferArray.iSize; ++i) {
        BGSLoadFormBuffer* ploadFormBuffer = this->FormBufferArray[i];
        if (ploadFormBuffer) {
            TESForm* pform = ploadFormBuffer->GetForm();
            if (pform) {
                pform->InitLoadGame(ploadFormBuffer);
            }
        }
    }
    if (abLoadModels) {
        TES::pInstance->QueueReferencesForAttachedCells();
        pIOManager->Resume();
        pIOManager->LoadAllQueuedPriorities();
        TESObjectCELL* pcell = pPlayer->GetParentCell();
        bhkWorld* pworld = pcell ? pcell->GetbhkWorld() : nullptr;
        if (pworld) {
            pworld->DeactivateAllIslands();
        }
        TES::pInstance->ActivateAllSubmergedReferencesIslands();
    }
    this->pQueuedSubBuffersMap->LoadAllQueuedSubBuffers();
    for (uint32_t i = 0; i < this->FormBufferArray.iSize; ++i) {
        BGSLoadFormBuffer* ploadFormBuffer = this->FormBufferArray[i];
        if (ploadFormBuffer) {
            TESForm* pform = ploadFormBuffer->GetForm();
            if (pform) {
                pform->FinishLoadGame(ploadFormBuffer);
                if (ploadFormBuffer->GetCellChanged()) {
                    Actor* pactor = static_cast<Actor*>(ploadFormBuffer->GetReference());
                    BGSChangeFlags kMoved(CHANGE_REFR_CELL_CHANGED);
                    AddChange(pactor, kMoved, true);
                    if (pactor->IsActor() && pactor->pCurrentProcess) {
                        pactor->MovetoLow();
                    }

                    this->UnloadForm(pactor, true);
                    TESObjectCELL* pparentcell = pactor->GetParentCell();
                    if (pparentcell) {
                        pparentcell->RemoveReference(pactor);
                    }
                    GarbageCollector::Add(pactor);
                }
                pform->SetStillLoading(false);
            }
            delete ploadFormBuffer;
        }
    }
    this->FormBufferArray.Clear(true);
    this->pChangedFormIDMap->RemoveAll();
    this->SetInitingForms(false);
}

bool BGSSaveLoadGame::GetPositioningPlayerCharacter() {
    return (uiGlobalFlags & GLOBAL_POSITIONING_PLAYER) != 0;
}

void BGSSaveLoadGame::SetDeferInitForms(bool abValue) {
    if (abValue) {
        uiGlobalFlags |= GLOBAL_DEFER_INIT_FORMS;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_DEFER_INIT_FORMS);
    }
}

bool BGSSaveLoadGame::GetPlayerLocationInvalid() {
    return (uiGlobalFlags & GLOBAL_PLAYER_LOCATION_INVALID) != 0;
}

uint8_t BGSSaveLoadGame::GetCurrentMinorVersion() {
    return cCurrentMinorVersion;
}

void BGSSaveLoadGame::SetSaveGameSaving(bool abValue) {
    if (abValue) {
        uiGlobalFlags |= GLOBAL_SAVEGAME_SAVING;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_SAVEGAME_SAVING);
    }
}

void BGSSaveLoadGame::SetSaveGameLoading(bool abValue) {
    if (abValue) {
        uiGlobalFlags |= GLOBAL_SAVEGAME_LOADING;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_SAVEGAME_LOADING);
    }
}

void BGSSaveLoadGame::SetPlayerLocationInvalid(bool abValue) {
    if (abValue) {
        uiGlobalFlags |= GLOBAL_PLAYER_LOCATION_INVALID;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_PLAYER_LOCATION_INVALID);
    }
}

uint8_t BGSSaveLoadGame::GetSaveMod(uint8_t aucFileIndex) {
    return cFileIndexArray[aucFileIndex];
}

bool BGSSaveLoadGame::GetSaveLoadFailed() {
    return (uiGlobalFlags & GLOBAL_SAVELOAD_FAILED) != 0;
}

void BGSSaveLoadGame::TogglePositioningPlayer(char acValue) {
    if (acValue) {
        uiGlobalFlags |= GLOBAL_POSITIONING_PLAYER;
    }
    else {
        uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_POSITIONING_PLAYER);
    }
}

void BGSSaveLoadGame::ClearFormID(uint32_t aiFormID) {
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    ClearForm(pForm);
}

void BGSSaveLoadGame::SaveGameWarningFunction(const char* apText, va_list aList) {
    pHistory->AddNoteVarArgs(apText, aList);
}

uint32_t BGSSaveLoadGame::GetChangedFormID(uint32_t aiOldFormID) {
    uint32_t uiNewFormID = 0;
    bool bFound = pChangedFormIDMap->NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(
        aiOldFormID,
        uiNewFormID
    );
    if (bFound) {
        return uiNewFormID;
    }

    return 0;
}

void BGSSaveLoadGame::CleanupExpiredData() {
    CellInitialData cellData;
    using ExpiredCells_t = NiTMap<int64_t, int32_t>;
    NiTMapItem<uint32_t, BGSFormChanges*>* it;
    bool brestore = SetThreadAllowChanges(false);
    ExpiredCells_t ExpiredCells(37u);
    uint32_t icurrentTime = appCalendar.GetHoursPassed();
    uint32_t iexpiredCellCount = 0;
    it = pChangesMap->GetFirstPos();
    while (it) {
        uint32_t iformID = 0;
        BGSFormChanges* pchanges = nullptr;
        pChangesMap->GetNext(it, iformID, pchanges);
        uint32_t ilocationID = iformID;
        if (pchanges->iChangeFlags.CheckFlags(CHANGE_CELL_DETACHTIME)) {
            int32_t idetachTime = 0;
            int32_t icellX = 0x7FFFFFFF;
            int32_t icellY = 0x7FFFFFFF;
            if (pchanges->UnloadedFormBuffer.pBuffer) {
                if (pchanges->UnloadedFormBuffer.GetFormType() == CELL_ID) {
                    INITIAL_DATA_TYPE einitialDataType = BGSSaveLoadInitialData::GetInitialDataType(
                        iformID,
                        CELL_ID,
                        pchanges->iChangeFlags
                    );
                    if (einitialDataType) {
                        char* pData = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
                        BGSSaveLoadInitialData::LoadInitialDataStruct(pData, einitialDataType, &cellData);
                        idetachTime = cellData.iDetachTime;
                        if (einitialDataType != INITIAL_DATA_INTERIOR_CELL) {
                            ilocationID = cellData.iWorldSpaceID;
                            icellX = cellData.iCellX;
                            icellY = cellData.iCellY;
                        }
                    }
                }
            }
            else {
                TESObjectCELL* pcell = static_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(iformID));
                if (pcell && pcell->GetFormType() == CELL_ID) {
                    idetachTime = pcell->GetDetachTime();
                    if (!pcell->GetInterior()) {
                        TESWorldSpace* pWorldSpace = pcell->GetWorldSpace();
                        ilocationID = pWorldSpace->iFormID;
                        icellX = pcell->GetDataX();
                        icellY = pcell->GetDataY();
                    }
                }
            }
            if (idetachTime) {
                uint32_t uiHoursDetached = icurrentTime - idetachTime;
                if (uiHoursDetached > TESObjectCELL::GetHoursToRespawnCell()) {
                    int64_t ilocationKey = ilocationID;
                    if (icellX != 0x7FFFFFFF) {
                        int64_t icellKey = TESObjectCELL::GetCoord(
                            static_cast<uint16_t>(icellX),
                            static_cast<uint16_t>(icellY)
                        );
                        ilocationKey |= icellKey << 32;
                    }
                    ExpiredCells.SetAt(ilocationKey, idetachTime);
                    ++iexpiredCellCount;
                }
            }
        }
    }
    if (iexpiredCellCount) {
        uint32_t iremoveHavokCount = 0;
        uint32_t iremoveHavokFullCount = 0;
        uint32_t iremoveHavokUnloadedPartialCount = 0;
        it = pChangesMap->GetFirstPos();
        while (it) {
            uint32_t iformID = 0;
            BGSFormChanges* pchanges = nullptr;
            pChangesMap->GetNext(it, iformID, pchanges);
            if (!pDataHandler->IsFormIDCreated(iformID)) {
                uint32_t ilocationID = 0;
                int32_t icellX = 0;
                int32_t icellY = 0;
                TESObjectREFR* pref = nullptr;
                ENUM_FORM_ID eformType = NO_FORM;
                if (pchanges->UnloadedFormBuffer.pBuffer) {
                    eformType = pchanges->UnloadedFormBuffer.GetFormType();
                }
                else {
                    TESForm* pform = TESForm::GetFormByNumericID(iformID);
                    if (pform) {
                        eformType = pform->GetFormType();
                        if (pform->IsReference()) {
                            pref = static_cast<TESObjectREFR*>(pform);
                        }
                    }
                    else {
                        int32_t ichangeFlags = pchanges->iChangeFlags.iFlags;
                        SaveGameWarning(
                            "During cleanup, form %08X with changeflags %08X and no unloaded form buffer "
                            "could not be found",
                            iformID,
                            ichangeFlags
                        );
                    }
                }
                if (eformType == REFR_ID) {
                    if (pchanges->UnloadedFormBuffer.pBuffer) {
                        INITIAL_DATA_TYPE einitialDataType = BGSSaveLoadInitialData::GetInitialDataType(
                            iformID,
                            eformType,
                            pchanges->iChangeFlags
                        );
                        if (einitialDataType) {
                            ReferenceInitialData refData;
                            char* pData = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
                            BGSSaveLoadInitialData::LoadInitialDataStruct(pData, einitialDataType, &refData);
                            ilocationID = refData.iLocationID;
                            icellX = FloatToInt(refData.Location.x) >> 12;
                            icellY = FloatToInt(refData.Location.y) >> 12;
                        }
                    }
                    else if (pref) {
                        TESObjectCELL* pparentCell = pref->GetParentCell();
                        TESWorldSpace* pworld = pref->GetWorldSpace();
                        if (pparentCell && pparentCell->GetInterior()) {
                            ilocationID = pparentCell->iFormID;
                        }
                        else if (pworld) {
                            ilocationID = pworld->iFormID;
                            NiPoint3 kLocation = pref->GetLocationOnReference();
                            icellX = FloatToInt(kLocation.x) >> 12;
                            icellY = FloatToInt(kLocation.y) >> 12;
                        }
                    }
                }
                if (ilocationID) {
                    int64_t ilocationKey = ilocationID;
                    if (icellX != 0x7FFFFFFF) {
                        int64_t icellKey = TESObjectCELL::GetCoord(
                            static_cast<uint16_t>(icellX),
                            static_cast<uint16_t>(icellY)
                        );
                        ilocationKey |= icellKey << 32;
                    }
                    int32_t idetachTime = 0;
                    if (ExpiredCells.GetAt(ilocationKey, idetachTime) &&
                        pchanges->iChangeFlags.CheckFlags(CHANGE_REFR_HAVOK_MOVE) &&
                        !pchanges->iChangeFlags.CheckFlags(CHANGE_REFR_MOVE)) {
                        ++iremoveHavokCount;
                        INITIAL_DATA_TYPE einitialDataType = BGSSaveLoadInitialData::GetInitialDataType(
                            iformID,
                            eformType,
                            pchanges->iChangeFlags
                        );
                        ReferenceInitialData movedRefData;
                        if (pchanges->UnloadedFormBuffer.pBuffer && einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
                            char* pData = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
                            BGSSaveLoadInitialData::LoadInitialDataStruct(
                                pData,
                                INITIAL_DATA_MOVED_REFERENCE,
                                &movedRefData
                            );
                            pReferencesMap->RemoveMovedReference(iformID);
                            TESForm* pform = TESForm::GetFormByNumericID(movedRefData.iLocationID);
                            if (pform && pform->GetFormType() == WRLD_ID) {
                                pReferencesMap
                                    ->RemoveReference(movedRefData.iLocationID, iformID, movedRefData.Location);
                            }
                            else {
                                pReferencesMap->RemoveReference_0(movedRefData.iLocationID, iformID);
                            }
                        }

                        if (pchanges->iChangeFlags.Equals(CHANGE_REFR_HAVOK_MOVE)) {
                            pChangesMap->RemoveChanges(iformID);
                            ++iremoveHavokFullCount;
                        }
                        else {
                            if (pchanges->UnloadedFormBuffer.pBuffer && einitialDataType) {
                                char* apBuffer = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
                                uint32_t iinitialDataSize = BGSSaveLoadInitialData::GetInitialDataTypeDataSize(
                                    einitialDataType
                                );
                                char* pbuffer = &apBuffer[iinitialDataSize];
                                uint32_t ihavokDataSize = 0;
                                uint32_t ihavokDataSizeTypeSize = LoadVariableSizedValueFromBuffer(
                                    pbuffer,
                                    ihavokDataSize
                                );
                                pbuffer = &pbuffer[ihavokDataSize + ihavokDataSizeTypeSize];
                                char* poriginalBuffer = pchanges->UnloadedFormBuffer.pBuffer;
                                uint8_t cversion = pchanges->UnloadedFormBuffer.GetVersion();
                                uint32_t idataSize = pchanges->UnloadedFormBuffer.GetSize();
                                uint32_t inewDataSize = idataSize - iinitialDataSize - ihavokDataSizeTypeSize -
                                    ihavokDataSize;
                                // The destructor only clears the pointer: the old buffer is freed below.
                                BGSUnloadedFormBuffer::DestructKeepBuffer(&pchanges->UnloadedFormBuffer);
                                pchanges->UnloadedFormBuffer.Initialize(pbuffer, inewDataSize, eformType, cversion);
                                ourFree(poriginalBuffer);
                                ++iremoveHavokUnloadedPartialCount;
                            }

                            pchanges->iChangeFlags.RemoveFlags(CHANGE_REFR_HAVOK_MOVE | CHANGE_REFR_CELL_CHANGED);
                        }
                        if (pref) {
                            pref->MoveToEditorLocation(nullptr);
                        }
                    }
                }
            }
        }
        if (iremoveHavokCount) {
            pHistory->AddNote(
                "CleanupExpiredData removed havok from %i refs: %i removed all changes and %i unloaded "
                "refs removed partial changes",
                iremoveHavokCount,
                iremoveHavokFullCount,
                iremoveHavokUnloadedPartialCount
            );
        }
    }
    ExpiredCells.RemoveAll_1();
    SetThreadAllowChanges(brestore);
}

bool BGSSaveLoadGame::LoadGame(BGSSaveLoadFile* apFile, bool abIgnoreMissingContent) {
    char cFullFileName[260];
    char cVersionInfo[260];
    char cSaveName[260];

    BGSSaveLoadHistory* pOldHistory = pHistory;
    pHistory = new BGSSaveLoadHistory;
    pSaveLoadManager->GetFullFileName(apFile->GetSaveFileName(), cFullFileName);
    pSaveLoadManager->GetVersionInfo(cVersionInfo, 0x104);
    pHistory->AddNote("***Loading game: %s (%s)", cFullFileName, cVersionInfo);
    LoadGameDataFromFileFunction(apFile, &cCurrentMinorVersion, 1);
    bool bPluginsLoaded = LoadPluginList(apFile);
    if (!bPluginsLoaded && !abIgnoreMissingContent) {
        BGSSaveLoadHistory* pNewHistory = pHistory;
        if (pNewHistory) {
            delete pNewHistory;
        }
        pHistory = pOldHistory;
        cCurrentMinorVersion = 0xFF;
        StartMenu::SetLoadMissingContent(true);
        return false;
    }
    if (pOldHistory) {
        delete pOldHistory;
    }
    pReferencesMap->Clear();
    pSaveLoadManager->StartLoadGame();
    pIOManager->Pause();
    SetSaveGameLoading(true);
    pPlayer->ClearTeammates();

    SaveGameFormInfo kHeader;
    apFile->Read(&kHeader, 0x6E);
    uint32_t iHeaderEnd = apFile->GetFilePosition();
    apFile->SeekSet(kHeader.iFormIDMapOffset);

    BGSSaveLoadFormIDMap* pOldFormIDMap = pFormIDMap;
    pFormIDMap = new BGSSaveLoadFormIDMap(0x13AF);
    pFormIDMap->Load(apFile);
    pWorldspaceFormIDMap->Load(apFile);

    BGSSaveLoadHistory* pLoadingHistory = pHistory;
    pHistory = new BGSSaveLoadHistory;
    pHistory->Load(apFile);
    pHistory->AddHistory(pLoadingHistory);
    if (pLoadingHistory) {
        delete pLoadingHistory;
    }
    apFile->SeekSet(iHeaderEnd);

    // The initial global data loads into a fresh changes map, which then holds the save's changes; the current
    // map goes back in place.
    pOldChangesMap = pChangesMap;
    pChangesMap = ::new BGSSaveLoadChangesMap;
    BGSSaveLoadGlobalData::RevertGlobalData(false);
    pDataHandler->SetIsLoading(true);
    BGSSaveLoadGlobalData::LoadGlobalData(apFile, kHeader.iInitialGlobalFormCount);
    BGSSaveLoadChangesMap* pLoadedChangesMap = pChangesMap;
    pChangesMap = pOldChangesMap;
    pOldChangesMap = nullptr;
    uint32_t iFormDataPosition = apFile->GetFilePosition();
    FormBufferArray.Clear(false);
    FormBufferArray.SetReservedSize(kHeader.iFormCount);
    SetGlobalAllowChanges(false);

    for (uint32_t i = 0; i < kHeader.iFormCount; ++i) {
        BGSLoadFormBuffer* pLoadFormBuffer = new BGSLoadFormBuffer;
        pLoadFormBuffer->LoadHeader(apFile);
        ENUM_FORM_ID eFormType = pLoadFormBuffer->GetFormType();
        uint32_t iFormID = pLoadFormBuffer->GetFormID();
        BGSChangeFlags kNewChangeFlags = pLoadFormBuffer->GetChangeFlags();
        BGSChangeFlags kOldChangeFlags;
        pChangesMap->GetChangeFlags(&kOldChangeFlags, iFormID);
        BGSChangeFlags kLoadedChangeFlags;
        pLoadedChangesMap->GetChangeFlags(&kLoadedChangeFlags, iFormID);
        if (static_cast<int32_t>(kLoadedChangeFlags)) {
            kOldChangeFlags.AddFlags(static_cast<int32_t>(kLoadedChangeFlags));
            pLoadedChangesMap->RemoveChanges(iFormID);
        }
        pLoadedChangesMap->SetChangeFlags(iFormID, kNewChangeFlags);
        pLoadFormBuffer->SetOldChangeFlags(kOldChangeFlags);
        TESForm* pForm = TESForm::GetFormByNumericID(iFormID);
        pLoadFormBuffer->SetForm(pForm);
        int32_t iLoaded = CheckInitialData(apFile, pLoadFormBuffer, pOldFormIDMap);
        pLoadFormBuffer->SkipData(apFile, iLoaded);
        pForm = pLoadFormBuffer->GetForm();
        kOldChangeFlags = pLoadFormBuffer->GetOldChangeFlags();
        bool bRemoveChanges = false;
        if (pForm) {
            bool bSkip = false;
            if (pForm->GetFormType() != eFormType) {
                bSkip = true;
            }
            else if (pForm->IsReference() && !static_cast<TESObjectREFR*>(pForm)->GetObjectReference()) {
                bSkip = true;
            }

            if (bSkip) {
                if (pDataHandler->IsFormIDCreated(iFormID)) {
                    ClearForm(pForm);
                    pLoadFormBuffer->SetForm(nullptr);
                    BGSChangeFlags kNoChanges(0);
                    pLoadFormBuffer->SetOldChangeFlags(kNoChanges);
                    bRemoveChanges = true;
                }
                else {
                    if (pForm->GetFormType() != eFormType) {
                        // The arguments are evaluated right to left.
                        const char* pCurrentType = pForm->GetFormTypeString();
                        const char* pSavedType = TESForm::GetFormTypeString(static_cast<uint8_t>(eFormType));
                        const char* pEditorID = pForm->GetFormEditorID();
                        SaveGameWarning(
                            "SAVELOAD: Form '%s' (%08X) was saved with form type %s, but currently has "
                            "form type %s.  Its loading will be skipped.",
                            pEditorID,
                            iFormID,
                            pSavedType,
                            pCurrentType
                        );
                    }
                    pLoadFormBuffer->SetSkipForm(true);
                }
            }
            else {
                pForm->Revert(pLoadFormBuffer);
                BGSChangeFlags kUnrevertible;
                BGSChangeFlags::GetUnrevertibleChanges(
                    &kUnrevertible,
                    kOldChangeFlags,
                    kNewChangeFlags,
                    eFormType,
                    true
                );
                if (static_cast<int32_t>(kUnrevertible)) {
                    if (!HandleUnrevertibleChanges(pForm, kUnrevertible, false)) {
                        pReconstructForms->AddForm(pForm, 0);
                        pLoadFormBuffer->SetReconstructedForm(true);
                    }
                }
                bRemoveChanges = true;
            }
        }
        else {
            bRemoveChanges = true;
        }
        if (bRemoveChanges) {
            pChangesMap->RemoveChanges(iFormID);
        }
        FormBufferArray.Add(pLoadFormBuffer);
    }

    RevertCurrentChanges(pLoadedChangesMap, pOldFormIDMap);
    BSShaderManager::ProcessAllQueuedLights();
    if (pOldFormIDMap) {
        delete pOldFormIDMap;
    }
    pDataHandler->SetIsLoading(false);
    PerformQueuedInitPackageLocations();
    apFile->SeekSet(iFormDataPosition);

    for (uint32_t i = 0; i < kHeader.iFormCount; ++i) {
        BGSLoadFormBuffer* pLoadFormBuffer = FormBufferArray[i];
        if (pLoadFormBuffer->GetSkipForm()) {
            pChangesMap->RemoveChanges(pLoadFormBuffer->GetFormID());
            pLoadFormBuffer->LoadHeader(apFile);
            pLoadFormBuffer->SkipData(apFile, 0);
            if (pLoadFormBuffer) {
                delete pLoadFormBuffer;
            }
            BGSLoadFormBuffer* pNone = nullptr;
            FormBufferArray.SetAt(i, pNone);
            continue;
        }
        uint32_t iFormID = pLoadFormBuffer->GetFormID();
        TESForm* pForm = pLoadFormBuffer->GetForm();
        if (pForm) {
            pLoadFormBuffer->LoadHeader(apFile);
            ENUM_FORM_ID eFormType = pLoadFormBuffer->GetFormType();
            bool bSkip = false;
            if (pForm->GetFormType() != eFormType) {
                // The arguments are evaluated right to left.
                const char* pCurrentType = pForm->GetFormTypeString();
                const char* pSavedType = TESForm::GetFormTypeString(static_cast<uint8_t>(eFormType));
                const char* pEditorID = pForm->GetFormEditorID();
                SaveGameWarning(
                    "SAVELOAD: Form '%s' (%08X) was saved with form type %s, but currently has form "
                    "type %s.  Its loading will be skipped.",
                    pEditorID,
                    iFormID,
                    pSavedType,
                    pCurrentType
                );
                bSkip = true;
            }
            else if (pForm->IsReference() && !static_cast<TESObjectREFR*>(pForm)->GetObjectReference()) {
                bSkip = true;
            }

            if (bSkip) {
                pChangesMap->RemoveChanges(pLoadFormBuffer->GetFormID());
                pLoadFormBuffer->SkipData(apFile, 0);
                if (pLoadFormBuffer) {
                    delete pLoadFormBuffer;
                }
                BGSLoadFormBuffer* pNone = nullptr;
                FormBufferArray.SetAt(i, pNone);
                continue;
            }
            // The byte count is tested as signed: -1 is a short read.
            if (static_cast<int32_t>(pLoadFormBuffer->LoadDataFromFile(apFile)) < 0 || GetSaveLoadFailed()) {
                pLoadFormBuffer->SkipData(apFile, 0);
                if (pLoadFormBuffer) {
                    delete pLoadFormBuffer;
                }
                BGSLoadFormBuffer* pNone = nullptr;
                FormBufferArray.SetAt(i, pNone);
                FormBufferArray.Clear(true);
                pPlayer->SetParentCell(nullptr);
                SetGlobalAllowChanges(true);
                SetSaveGameLoading(false);
                return false;
            }
            BGSSaveLoadInitialData::LoadInitialData(pLoadFormBuffer, pForm, pLoadFormBuffer->GetChangeFlags());
            if (pLoadFormBuffer->GetForm() && !GetSaveLoadFailed()) {
                pForm->SetStillLoading(true);
                pForm->LoadGame(pLoadFormBuffer);
            }
        }
        else {
            BGSUnloadedFormBuffer kUnloadedBuffer;
            kUnloadedBuffer.Load(apFile);
            pChangesMap->SetChanges(iFormID, pLoadFormBuffer->GetChangeFlags(), kUnloadedBuffer);
        }
        pLoadFormBuffer->ClearBuffer();
    }

    apFile->SeekCurrent(kHeader.iSecondaryDataSize);
    BGSSaveLoadGlobalData::InitInitialGlobalData();
    InitForms(true);
    BGSSaveLoadGlobalData::FinishLoadInitialGlobalData();
    BGSSaveLoadGlobalData::LoadGlobalData(apFile, kHeader.iFinalGlobalFormCount);
    cCurrentMinorVersion = 0xFF;
    BGSSaveLoadManager::CreateSystemSaveName(cSaveName);
    pSaveLoadManager->pSaveName.Set(cSaveName, 0);
    pHistory->AddNote("---Finished loading game: %s", cFullFileName);
    SetGlobalAllowChanges(true);
    appCalendar.SetGameLoaded();
    SetSaveGameLoading(false);
    return true;
}

bool BGSSaveLoadGame::LoadPluginList(BGSSaveLoadFile* apFile) {
    uint8_t ccurrentCount = pDataHandler->CountCompiled();
    BGSLoadGameBuffer loadBuffer;
    loadBuffer.Load(apFile);
    bool bpluginsComplete = true;
    ourMemset(cFileIndexArray, 255, 255u);
    ourMemset(cReverseFileIndexArray, 255, 255u);
    uint8_t ccount = 0;
    loadBuffer.LoadData(&ccount, sizeof(ccount));
    for (uint32_t i = 0; i < ccount; ++i) {
        char cFileName[264];
        loadBuffer.LoadString(cFileName);
        bool bfound = false;
        for (uint32_t ii = 0; ii < ccurrentCount; ++ii) {
            TESFile* pfile = pDataHandler->GetCompiledFile(static_cast<int32_t>(ii));
            if (!BSstricmp(cFileName, pfile->m_Filename)) {
                cFileIndexArray[i] = ii;
                cReverseFileIndexArray[ii] = i;
                bfound = true;
                break;
            }
        }
        if (!bfound) {
            bpluginsComplete = false;
            cFileIndexArray[i] = -1;
            SaveGameWarning(
                "SAVELOAD: Cannot find file %s referenced in the save game.  Errors may result.",
                cFileName
            );
        }
    }
    return bpluginsComplete;
}

// Created and moved references carry their initial data in the save. A reference whose cell is loaded is recreated
// (created) or queued for reconstruction (moved); otherwise it is recorded in the references map and the loaded form,
// if any, is deleted. A reference whose changes do not say "moved" is deleted when its editor cell is not loaded, or
// queued for reconstruction from its old buffer when it has no form.
uint32_t BGSSaveLoadGame::CheckInitialData(
    BGSSaveLoadFile* apFile,
    BGSLoadFormBuffer* apLoadGameBuffer,
    BGSSaveLoadFormIDMap* apOldFormIDMap
) {
    uint32_t iLoaded = 0;
    uint32_t eFormType = apLoadGameBuffer->GetFormType();
    BGSChangeFlags kChangeFlags = apLoadGameBuffer->GetChangeFlags();
    BGSChangeFlags kOldChangeFlags = apLoadGameBuffer->GetOldChangeFlags();
    uint32_t iFormID = apLoadGameBuffer->GetFormID();
    TESForm* pForm = apLoadGameBuffer->GetForm();
    int32_t eType = BGSSaveLoadInitialData::GetInitialDataType(iFormID, static_cast<int32_t>(eFormType), kChangeFlags);
    if (eType == INITIAL_DATA_CREATED_REFERENCE || eType == INITIAL_DATA_MOVED_REFERENCE) {
        ReferenceInitialData kData;
        iLoaded = BGSSaveLoadInitialData::LoadInitialDataStruct(apFile, static_cast<INITIAL_DATA_TYPE>(eType), &kData);
        TESObjectREFR* pRef = nullptr;
        if (pForm) {
            bool bClear = false;
            if (!pForm->IsReference()) {
                if (eType == INITIAL_DATA_CREATED_REFERENCE) {
                    bClear = true;
                }
            }
            else {
                pRef = static_cast<TESObjectREFR*>(pForm);
                if (eType == INITIAL_DATA_CREATED_REFERENCE &&
                    pRef->GetObjectReference()->GetFormID() != kData.iBoundID) {
                    bClear = true;
                }
            }
            if (bClear) {
                ClearForm(pForm);
                apLoadGameBuffer->SetForm(nullptr);
                BGSChangeFlags kNoChanges(0);
                apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
                pForm = nullptr;
                pRef = nullptr;
            }
        }
        TESObjectCELL* pCell = nullptr;
        TESWorldSpace* pWorld = nullptr;
        kData.GetLocationCellAndWorld(pCell, pWorld);
        if ((pCell && TES::pInstance->IsCellLoaded(pCell, false)) ||
            (kData.cCreatedReferenceFlags & CREATED_REFERENCE_PERSISTENT) != 0) {
            if (eType == INITIAL_DATA_CREATED_REFERENCE) {
                if (pForm) {
                    BGSChangeFlags kUnrevertible;
                    BGSChangeFlags::GetUnrevertibleChanges(
                        &kUnrevertible,
                        kOldChangeFlags,
                        kChangeFlags,
                        eFormType,
                        true
                    );
                    if (static_cast<int32_t>(kUnrevertible)) {
                        DeleteForm(pForm);
                        pForm = nullptr;
                        BGSChangeFlags kNoChanges(0);
                        apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
                    }
                }
                pForm = BGSSaveLoadInitialData::CreateReference(iFormID, &kData, pForm);
                if (pForm) {
                    apLoadGameBuffer->SetForm(pForm);
                }
                else {
                    apLoadGameBuffer->SetSkipForm(true);
                }
            }
            else if (eType == INITIAL_DATA_MOVED_REFERENCE && !pForm) {
                kData.GetOriginalLocationCellAndWorld(pCell, pWorld);
                if (pCell) {
                    pReconstructForms->AddReference(pCell, iFormID, 0);
                }
                else {
                    pReconstructForms->AddReference(pWorld, kData.iOriginalCellX, kData.iOriginalCellY, iFormID, 0);
                }
                apLoadGameBuffer->SetReconstructedForm(true);
                apLoadGameBuffer->SetFormPending(true);
            }
        }
        else if (!pRef || (!pRef->GetRefPersists() && pRef != pPlayer)) {
            if (pCell && pCell->GetInterior()) {
                pReferencesMap->AddReference(kData.iLocationID, iFormID);
                if (eType == INITIAL_DATA_MOVED_REFERENCE) {
                    pReferencesMap->AddMovedReference(iFormID);
                }
            }
            else if (pWorld) {
                pReferencesMap->AddReference(kData.iLocationID, iFormID, kData.Location);
                if (eType == INITIAL_DATA_MOVED_REFERENCE) {
                    pReferencesMap->AddMovedReference(iFormID);
                }
            }
            else {
                SaveGameWarning(
                    "CELLS: Worldspace or Cell %08X could not be found while loading a %s reference.  Its "
                    "loading will be skipped.",
                    kData.iLocationID,
                    eType == INITIAL_DATA_CREATED_REFERENCE ? "created" : "moved"
                );
                apLoadGameBuffer->SetSkipForm(true);
            }
            if (pRef) {
                if (pRef->GetParentCell()) {
                    pRef->GetParentCell()->RemoveReference(pRef);
                }
                DeleteForm(pForm);
                pForm = nullptr;
                apLoadGameBuffer->SetForm(nullptr);
            }
        }
    }
    else {
        int32_t eOldType = BGSSaveLoadInitialData::GetInitialDataType(
            iFormID,
            static_cast<int32_t>(eFormType),
            kOldChangeFlags
        );
        if (eOldType == INITIAL_DATA_MOVED_REFERENCE) {
            if (!pForm) {
                BGSUnloadedFormBuffer kBuffer;
                pChangesMap->GetChangeBuffer(&kBuffer, iFormID);
                if (static_cast<char*>(kBuffer)) {
                    BGSSaveLoadFormIDMap* pSavedFormIDMap = pFormIDMap;
                    pFormIDMap = apOldFormIDMap;
                    ReferenceInitialData kData;
                    BGSSaveLoadInitialData::LoadInitialDataStruct(
                        kBuffer.GetAdvancedBuffer(),
                        static_cast<INITIAL_DATA_TYPE>(eOldType),
                        &kData
                    );
                    TESObjectCELL* pCell = nullptr;
                    TESWorldSpace* pWorld = nullptr;
                    kData.GetOriginalLocationCellAndWorld(pCell, pWorld);
                    if (pCell && TES::pInstance->IsCellLoaded(pCell, false)) {
                        pReconstructForms->AddReference(pCell, iFormID, 0);
                        apLoadGameBuffer->SetReconstructedForm(true);
                        apLoadGameBuffer->SetFormPending(true);
                    }
                    pFormIDMap = pSavedFormIDMap;
                }
            }
            else if (pForm->IsReference()) {
                TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pForm);
                if (!pRef->GetRefPersists()) {
                    TESForm* pLocation = nullptr;
                    NiPoint3 kLocation = NiPoint3::ZERO;
                    NiPoint3 kAngle = NiPoint3::ZERO;
                    bool bDelete = false;
                    if (!pRef->GetEditorLocation(kLocation, kAngle, pLocation, nullptr)) {
                        bDelete = true;
                    }
                    else {
                        TESObjectCELL* pCell = nullptr;
                        if (pLocation->GetFormType() == WRLD_ID) {
                            pCell = static_cast<TESWorldSpace*>(pLocation)->GetCellFromWorldCoord(kLocation);
                        }
                        else {
                            pCell = static_cast<TESObjectCELL*>(pLocation);
                        }
                        if (!pCell || !TES::pInstance->IsCellLoaded(pCell, false)) {
                            bDelete = true;
                        }
                    }
                    if (bDelete) {
                        DeleteForm(pForm);
                        apLoadGameBuffer->SetForm(nullptr);
                        BGSChangeFlags kNoChanges(0);
                        apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
                        pForm = nullptr;
                    }
                }
            }
        }
    }
    return iLoaded;
}

bool BGSSaveLoadGame::HandleUnrevertibleChanges(
    TESForm*& arpForm,
    BGSChangeFlags aiUnrevertibleChanges,
    bool abRevertOnly
) {
    if (arpForm == pPlayer) {
        return true;
    }
    if (arpForm->IsReference()) {
        Actor* pactor = static_cast<Actor*>(arpForm);
        if (aiUnrevertibleChanges.CheckFlags(CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE | CHANGE_REFR_CELL_CHANGED)) {
            uint32_t uiFormID = arpForm->iFormID;
            if (pDataHandler->IsFormIDCreated(uiFormID) || pactor->MoveToEditorLocation(nullptr)) {
                aiUnrevertibleChanges.RemoveFlags(CHANGE_REFR_MOVE | CHANGE_REFR_HAVOK_MOVE | CHANGE_REFR_CELL_CHANGED);
                if (!aiUnrevertibleChanges.iFlags && pactor->IsActor()) {
                    QueueInitPackageLocations(pactor);
                }
            }
            else if (!abRevertOnly || pactor->GetRefPersists()) {
                const char* pcName;
                if (pactor->GetFormEditorIDLength()) {
                    pcName = pactor->GetFormEditorID();
                }
                else {
                    pcName = pactor->GetFullName();
                }
                int32_t iChanges = aiUnrevertibleChanges.iFlags;
                const char* pcFormTypeString = pactor->GetObjectReference()->GetFormTypeString();
                SaveGameWarning(
                    "Reference %s %08X with type %s and with changes %08X could not revert its changed "
                    "location",
                    pcName,
                    pactor->iFormID,
                    pcFormTypeString,
                    iChanges
                );
            }
            else {
                DeleteForm(pactor);
                arpForm = nullptr;
            }
        }
    }
    return aiUnrevertibleChanges.iFlags == 0;
}

// A projectile is unloaded only when it is permanent.
bool BGSSaveLoadGame::ShouldUnloadForm(TESForm* apForm) {
    if (apForm->GetTemporary()) {
        return false;
    }
    if (apForm->IsReference()) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
        if (pRef->IsProjectile()) {
            Projectile* pProjectile = static_cast<Projectile*>(pRef);
            if (!pProjectile->IsPermanent()) {
                return false;
            }
        }
    }
    return true;
}

// Allocates the singleton, installs its constructor result, then initializes form types.
void BGSSaveLoadGame::Create() {
    BGSSaveLoadGame* pNewGame = new BGSSaveLoadGame;
    pSaveLoadGame = pNewGame;
    BGSSaveLoadFormInfo::InitSaveGameFormTypes();
}

void BGSSaveLoadGame::Destroy() {
    if (pSaveLoadGame) {
        delete pSaveLoadGame;
    }
    pSaveLoadGame = nullptr;
}

bool BGSLoadFormBuffer::GetSkipForm() {
    return (iFlags & LOAD_FORM_SKIP_FORM) != 0;
}

void BGSLoadFormBuffer::SetSkipForm(bool abValue) {
    if (abValue) {
        iFlags |= LOAD_FORM_SKIP_FORM;
    }
    else {
        iFlags &= ~static_cast<uint32_t>(LOAD_FORM_SKIP_FORM);
    }
}

uint32_t BGSLoadFormBuffer::GetFormID() {
    return iFormID;
}

void BGSSaveGameBuffer::ClearPosition() {
    iBufferPosition = 0;
    iDebugOverhead = 0;
}

char* BGSSaveLoadFile::GetSaveFileName() {
    return pSaveFileName;
}

ReferenceInitialData::ReferenceInitialData() {}

void BGSSaveLoadReferencesMap::AddMovedReference(uint32_t aiFormID) {
    MovedReferencesMap.SetAt(aiFormID, aiFormID);
}

void BGSSaveLoadReferencesMap::AddReference(uint32_t aiCellID, uint32_t aiReferenceID) {
    InteriorReferencesMap.AddReference(aiCellID, aiReferenceID);
}

bool BGSSaveLoadReferencesMap::RemoveMovedReference(uint32_t aiFormID) {
    return MovedReferencesMap.RemoveAt(aiFormID);
}
