#include "Fallout Shared/BGSHunger.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Reads the DATA chunk (the stage's hunger level and the form ID of its sickness spell) and the editor ID. GetTESForm's
// result is stored as a byte and compared zero-extended, hence the uint8_t.
bool BGSHungerStage::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != HUNG_ID) {
        return false;
    }
    static_cast<TESForm*>(this)->ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        if (eChunk == DATA_ID) {
            // The data is this form's own component.
            LoadData(apFile, static_cast<char*>(static_cast<void*>(GetData())), 8);
            // The file's record is 8 bytes, the sickness's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(
                GetData(),
                1,
                8,
                sizeof(BGSHungerStageData),
                {{4, offsetof(BGSHungerStageData, pSickness)}}
            );
        }
        else if (eChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pEditorID, 0x200);
            static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
        }
        else if (eChunk == OBND_ID) {
            static_cast<TESForm*>(this)->LoadObjectBound(apFile);
        }

        if (eChunk == DATA_ID && apFile->QEndian()) {
            data.Endian();
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

// The hunger level and the sickness spell swapped for a big-endian file.
void BGSHungerStageData::Endian() {
    BSEndian(iFOOD, false);
    BSEndian(pSickness, false);
}

void BGSHungerStageData::Clear() {
    iFOOD = 0;
    pSickness = nullptr;
}

BGSHungerStage::BGSHungerStage() {
    data.Clear();
    SetFormType(119);
    BGSHungerStage::InitializeData();
}

BGSHungerStage::~BGSHungerStage() {
    BGSHungerStage::ClearData();
}

void BGSHungerStage::InitializeData() {
    BGSHungerStageData* pData = GetData();
    pData->Clear();
}

void BGSHungerStage::ClearData() {
    BGSHungerStage::RemoveStage(this);
}

void BGSHungerStage::Save() {
    StartForm();
    SpellItem* pSickness = GetSickness();
    SpellItem* pSicknessID = nullptr;
    if (GetSickness()) {
        // The file stores the sickness's form ID in the pointer field's place.
        pSicknessID = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(GetSickness()->GetFormID()));
    }
    data.pSickness = pSicknessID;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    // The file keeps the form IDs as 4 bytes; pack the record to its layout for the write and restore it after.
    TESFile::NarrowFormIDRecords(
        &data,
        1,
        8,
        sizeof(BGSHungerStageData),
        {{4, offsetof(BGSHungerStageData, pSickness)}}
    );
    __SaveData(&data, 8u);
    TESFile::WidenFormIDRecords(
        &data,
        1,
        8,
        sizeof(BGSHungerStageData),
        {{4, offsetof(BGSHungerStageData, pSickness)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    data.pSickness = pSickness;
    CloseForm();
}

void BGSHungerStage::InitItem() {
    if (!GetInitialized()) {
        // Load keeps the sickness's form ID in the SpellItem* field until here, so the pointer is read back as a form
        // ID.
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pSickness));
        if (uiFormID) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiFormID, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            data.pSickness = dynamic_cast<SpellItem*>(pForm);
        }
        SetInitialized(true);
        BGSHungerStage::AddStage(this);
    }
}

void BGSHungerStage::Copy(TESForm* apCopy) {
    BGSHungerStage* pCopy = dynamic_cast<BGSHungerStage*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        ourMemcpy(&data, &pCopy->data, sizeof(data));
    }
}

bool BGSHungerStage::Compare(TESForm* apCompare) {
    BGSHungerStage* pStage = dynamic_cast<BGSHungerStage*>(apCompare);
    if (!pStage) {
        return true;
    }
    if (TESForm::CompareAllComponents(pStage)) {
        return true;
    }
    BGSHungerStageData* pOtherData = pStage->GetData();
    return std::memcmp(GetData(), pOtherData, sizeof(BGSHungerStageData)) != 0;
}

SpellItem* BGSHungerStage::GetSickness() {
    return data.pSickness;
}

void BGSHungerStage::AddStage(BGSHungerStage* apNewStage) {
    uint8_t ucNewStagesCount = BGSHungerStage::iNumStages + 1;
    BGSHungerStage** ppNewStages = BGSHungerStage::AllocateStages(static_cast<uint8_t>(BGSHungerStage::iNumStages + 1));
    int32_t iCurrentStage = 0;
    char cAdded = 0;
    for (uint32_t i = 0; i < BGSHungerStage::iNumStages; ++i) {
        if (apNewStage->data.iFOOD > BGSHungerStage::pStagesArray[i]->data.iFOOD && !cAdded) {
            ppNewStages[iCurrentStage++] = apNewStage;
            cAdded = 1;
        }
        ppNewStages[iCurrentStage++] = BGSHungerStage::pStagesArray[i];
    }
    if (!cAdded) {
        ppNewStages[iCurrentStage] = apNewStage;
    }
    ourFree(BGSHungerStage::pStagesArray);
    BGSHungerStage::iNumStages = ucNewStagesCount;
    BGSHungerStage::pStagesArray = ppNewStages;
}

void BGSHungerStage::RemoveStage(BGSHungerStage* apStage) {
    if (BGSHungerStage::iNumStages) {
        uint8_t ucInArray = 0;
        for (uint32_t i = 0; !ucInArray && i < static_cast<uint32_t>(BGSHungerStage::iNumStages); ++i) {
            ucInArray = BGSHungerStage::pStagesArray[i] == apStage;
        }
        if (ucInArray) {
            uint32_t uiNewStagesCount = static_cast<uint32_t>(BGSHungerStage::iNumStages - 1);
            BGSHungerStage** ppNewStages = BGSHungerStage::AllocateStages(BGSHungerStage::iNumStages - 1);
            if (ppNewStages) {
                int32_t iCurrentStage = 0;
                for (uint32_t j = 0; j < static_cast<uint32_t>(BGSHungerStage::iNumStages); ++j) {
                    if (BGSHungerStage::pStagesArray[j] != apStage) {
                        ppNewStages[iCurrentStage++] = BGSHungerStage::pStagesArray[j];
                    }
                }
            }
            ourFree(BGSHungerStage::pStagesArray);
            BGSHungerStage::iNumStages = static_cast<int32_t>(uiNewStagesCount);
            BGSHungerStage::pStagesArray = ppNewStages;
        }
    }
}

BGSHungerStage* BGSHungerStage::FindStageForRadLevel(float afRads) {
    BGSHungerStage* pStage = nullptr;
    for (uint32_t i = 0; i < BGSHungerStage::iNumStages && !pStage; ++i) {
        BGSHungerStage* pCurrentStage = BGSHungerStage::GetStage(i);
        if (pCurrentStage && afRads >= static_cast<double>(pCurrentStage->data.iFOOD)) {
            pStage = pCurrentStage;
        }
    }
    return pStage;
}

BGSHungerStage* BGSHungerStage::GetStage(uint32_t aiIndex) {
    BGSHungerStage* pStage = aiIndex < static_cast<uint32_t>(iNumStages) ? pStagesArray[aiIndex] : nullptr;
    return pStage;
}

BGSHungerStage** BGSHungerStage::AllocateStages(uint32_t aiStagesCount) {
    if (!aiStagesCount) {
        return nullptr;
    }
    // new BGSHungerStage*[n]: the size requested is SIZE_MAX when it would overflow.
    BGSHungerStage** ppStages = static_cast<BGSHungerStage**>(ourMalloc(
        aiStagesCount <= SIZE_MAX / sizeof(BGSHungerStage*) ? sizeof(BGSHungerStage*) * aiStagesCount : SIZE_MAX
    ));
    for (uint32_t i = 0; i < aiStagesCount; ++i) {
        ppStages[i] = nullptr;
    }
    return ppStages;
}
