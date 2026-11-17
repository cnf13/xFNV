#include "Fallout Shared/BGSHunger.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

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
#if FNVD_64BIT
            // The file's record is 8 bytes, the sickness's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(
                GetData(), 1, 8, sizeof(BGSHungerStageData), {{4, offsetof(BGSHungerStageData, pSickness)}}
            );
#endif
        }
        else if (eChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
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
    ENDIAN(iFOOD, false);
    ENDIAN(pSickness, false);
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
    SpellItem* pSicknessID;
    if (GetSickness()) {
        // The file stores the sickness's form ID in the pointer field's place.
        pSicknessID = reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(GetSickness()->GetFormID()));
    }
    else {
        pSicknessID = nullptr;
    }
    data.pSickness = pSicknessID;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, 8u);
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
    BGSHungerStage* pcopy = dynamic_cast<BGSHungerStage*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        ourMemcpy(&data, &pcopy->data, sizeof(data));
    }
}

bool BGSHungerStage::Compare(TESForm* apCompare) {
    BGSHungerStage* v6 = dynamic_cast<BGSHungerStage*>(apCompare);
    if (!v6) {
        return true;
    }
    if (TESForm::CompareAllComponents(v6)) {
        return true;
    }
    BGSHungerStageData* formList = v6->GetData();
    BGSHungerStageData* v3 = GetData();
    return (std::memcmp(v3, formList, sizeof(BGSHungerStageData)) != 0);
}

SpellItem* BGSHungerStage::GetSickness() {
    return data.pSickness;
}

void BGSHungerStage::AddStage(BGSHungerStage* apNewStage) {
    uint8_t v3 = BGSHungerStage::iNumStages + 1;
    BGSHungerStage** Stages = BGSHungerStage::AllocateStages(static_cast<uint8_t>(BGSHungerStage::iNumStages + 1));
    int32_t v4 = 0;
    char v5 = 0;
    for (uint32_t i = 0; i < BGSHungerStage::iNumStages; ++i) {
        BGSHungerStageData* Data = &apNewStage->data;
        if (Data->iFOOD > (&BGSHungerStage::pStagesArray[i]->data)->iFOOD && !v5) {
            Stages[v4++] = apNewStage;
            v5 = 1;
        }
        Stages[v4++] = BGSHungerStage::pStagesArray[i];
    }
    if (!v5) {
        Stages[v4] = apNewStage;
    }
    ourFree(BGSHungerStage::pStagesArray);
    BGSHungerStage::iNumStages = v3;
    BGSHungerStage::pStagesArray = Stages;
}

void BGSHungerStage::RemoveStage(BGSHungerStage* apStage) {
    uint32_t j;
    int32_t v2;
    uint32_t i;
    uint8_t v4;
    uint32_t aiStagesCount;
    BGSHungerStage** stages;
    if (BGSHungerStage::iNumStages) {
        v4 = 0;
        for (i = 0; (!v4) && (i < ((uint32_t)BGSHungerStage::iNumStages)); ++i) {
            v4 = (uint8_t)(BGSHungerStage::pStagesArray[i] == apStage);
        }
        if (v4) {
            aiStagesCount = (uint32_t)(BGSHungerStage::iNumStages - 1);
            stages = BGSHungerStage::AllocateStages(BGSHungerStage::iNumStages - 1);
            if (stages) {
                v2 = 0;
                for (j = 0; j < ((uint32_t)BGSHungerStage::iNumStages); ++j) {
                    if (BGSHungerStage::pStagesArray[j] != apStage) {
                        stages[v2++] = BGSHungerStage::pStagesArray[j];
                    }
                }
            }
            ourFree(BGSHungerStage::pStagesArray);
            BGSHungerStage::iNumStages = (int32_t)aiStagesCount;
            BGSHungerStage::pStagesArray = stages;
        }
    }
}

BGSHungerStage* BGSHungerStage::FindStageForRadLevel(float afRads) {
    BGSHungerStage* pstage = nullptr;
    for (uint32_t i = 0; i < BGSHungerStage::iNumStages && !pstage; ++i) {
        BGSHungerStage* pcurrentStage = BGSHungerStage::GetStage(i);
        if (pcurrentStage) {
            if (afRads >= static_cast<double>(pcurrentStage->data.iFOOD)) {
                pstage = pcurrentStage;
            }
        }
    }
    return pstage;
}

BGSHungerStage* BGSHungerStage::GetStage(uint32_t aiIndex) {
    BGSHungerStage* pStage;
    if (aiIndex < static_cast<uint32_t>(iNumStages)) {
        pStage = pStagesArray[aiIndex];
    }
    else {
        pStage = nullptr;
    }
    return pStage;
}

BGSHungerStage** BGSHungerStage::AllocateStages(uint32_t aiStagesCount) {
    if (!aiStagesCount) {
        return nullptr;
    }
    // new BGSHungerStage*[n]: the size requested is SIZE_MAX when it would overflow.
    BGSHungerStage** v3 = static_cast<BGSHungerStage**>(ourMalloc(
        aiStagesCount <= SIZE_MAX / sizeof(BGSHungerStage*) ? sizeof(BGSHungerStage*) * aiStagesCount : SIZE_MAX
    ));
    for (uint32_t i = 0; i < aiStagesCount; ++i) {
        v3[i] = nullptr;
    }
    return v3;
}
