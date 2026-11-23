#include "Fallout Shared/BGSDefaultObjectManager.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/DEFAULT_OBJECT_DATA.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// Reads the DOBJ record: the DATA chunk holds the default objects' form IDs, resolved later by InitItem. The record
// type is kept in a byte before comparing it.
bool BGSDefaultObjectManager::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != DOBJ_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        uint32_t uiChunkID = apFile->GetTESChunk();
        if (uiChunkID == NO_CHUNK) {
            break;
        }
        if (uiChunkID == DATA_ID) {
            apFile->GetChunkData(pObjectArray, sizeof(pObjectArray));
            // The file keeps a 4-byte form ID for each object, resolved by InitItem.
            TESFile::WidenFormIDRecords(
                pObjectArray,
                apFile->GetChunkSize() / 4 < DO_COUNT ? apFile->GetChunkSize() / 4 : DO_COUNT,
                4,
                sizeof(TESForm*),
                {{0, 0}}
            );
            if (apFile->QEndian()) {
                for (int32_t i = 0; i < DO_COUNT; ++i) {
                    ENDIAN(pObjectArray[i], false);
                }
            }
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    SetInitialized(false);
    return true;
}

BGSDefaultObjectManager::BGSDefaultObjectManager() : TESForm() {
    cFormType = 100;
    TESForm::SetFormID(49u, true);
    TESForm::SetFormEditorID("DefaultObjectManager");
    BGSDefaultObjectManager::InitializeData();
}

BGSDefaultObjectManager::~BGSDefaultObjectManager() {}

void BGSDefaultObjectManager::InitializeData() {
    ourMemset(pObjectArray, 0, sizeof(pObjectArray));
}

// Writes the DOBJ record: one DATA chunk with the form ID of every default object (0 for an empty slot).
void BGSDefaultObjectManager::Save() {
    StartForm();
    uint32_t uiFormIDs[DO_COUNT];
    for (int32_t i = 0; i < DO_COUNT; ++i) {
        uiFormIDs[i] = pObjectArray[i] ? pObjectArray[i]->GetFormID() : 0;
    }
    AddChunkArray(DATA_ID, uiFormIDs, DO_COUNT);
    CloseForm();
}

void BGSDefaultObjectManager::InitItem() {
    if (!GetInitialized()) {
        for (int32_t i = 0; i < 34; ++i) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pObjectArray[i]));
            if (itempint) {
                TESFile* pFile = GetFile(-1);
                TESForm::AddCompileIndex(itempint, pFile);
                pObjectArray[i] = TESForm::GetFormByNumericID(itempint);
                if (pObjectArray[i]) {
                    int32_t iFormType = pObjectArray[i]->GetFormType();
                    if (iFormType != kDefaultObjectData[i].eType) {
                        pObjectArray[i] = nullptr;
                    }
                }
                if (!pObjectArray[i]) {
                    Warning(
                        "Unable to find valid '%s' default object for ID (%08X).",
                        kDefaultObjectData[i].pName,
                        itempint
                    );
                }
            }
        }
        SetInitialized(true);
    }
}

BGSDefaultObjectManager* BGSDefaultObjectManager::Instance() {
    if (BGSDefaultObjectManager::pSingleton) {
        return BGSDefaultObjectManager::pSingleton;
    }
    BGSDefaultObjectManager* pManager = new BGSDefaultObjectManager;
    BGSDefaultObjectManager::pSingleton = pManager;
    return BGSDefaultObjectManager::pSingleton;
}

TESForm* BGSDefaultObjectManager::GetDefaultObject(DEFAULT_OBJECT aeObject) {
    if (BGSDefaultObjectManager::Instance()->TESForm::GetInitialized()) {
        return BGSDefaultObjectManager::Instance()->pObjectArray[aeObject];
    }

    return nullptr;
}

void BGSDefaultObjectManager::ReleaseInstance() {
    if (BGSDefaultObjectManager::pSingleton) {
        delete BGSDefaultObjectManager::pSingleton;
        BGSDefaultObjectManager::pSingleton = nullptr;
    }
}
