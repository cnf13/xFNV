#include "Fallout Shared/BGSSleepDeprevation.h"

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

// Reads the DATA chunk (the stage's sleep deprivation and the form ID of its sickness spell) and the editor ID.
// GetTESForm's result is stored as a byte and compared zero-extended, hence the uint8_t.
bool BGSSleepDeprevationStage::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != SLPD_ID) {
        return false;
    }
    ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        if (eChunk == DATA_ID) {
            LoadData(apFile, GetData(), 8);
            // The file's record is 8 bytes, the sickness's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(
                GetData(),
                1,
                8,
                sizeof(BGSSleepDeprevationStageData),
                {{4, offsetof(BGSSleepDeprevationStageData, pSickness)}}
            );
        }
        else if (eChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            uint32_t uiChunkSize = apFile->GetChunkSize();
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
            // An empty chunk leaves the buffer unwritten (see above).
            pEditorID[0] = '\0';
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

void BGSSleepDeprevationStageData::Endian() {
    BSEndian(iSleep, false);
    BSEndian(pSickness, false);
}

void BGSSleepDeprevationStageData::Clear() {
    iSleep = 0;
    pSickness = nullptr;
}

BGSSleepDeprevationStage::BGSSleepDeprevationStage() {
    SetFormType(SLPD_ID);
    BGSSleepDeprevationStage::InitializeData();
}

BGSSleepDeprevationStage::~BGSSleepDeprevationStage() {
    BGSSleepDeprevationStage::ClearData();
}

void BGSSleepDeprevationStage::InitializeData() {
    BGSSleepDeprevationStageData* pData = GetData();
    pData->Clear();
}

void BGSSleepDeprevationStage::ClearData() {
    BGSSleepDeprevationStage::RemoveStage(this);
}

void BGSSleepDeprevationStage::Save() {
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
    // The file keeps the form IDs as 4 bytes; pack the record to its layout for the write and restore it after.
    TESFile::NarrowFormIDRecords(
        &data,
        1,
        8,
        sizeof(BGSSleepDeprevationStageData),
        {{4, offsetof(BGSSleepDeprevationStageData, pSickness)}}
    );
    __SaveData(&data, 8u);
    TESFile::WidenFormIDRecords(
        &data,
        1,
        8,
        sizeof(BGSSleepDeprevationStageData),
        {{4, offsetof(BGSSleepDeprevationStageData, pSickness)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    data.pSickness = pSickness;
    CloseForm();
}

void BGSSleepDeprevationStage::InitItem() {
    if (!GetInitialized()) {
        // Load keeps the sickness's form ID in the SpellItem* field until here, so it is read back as a uint32_t.
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pSickness));
        if (uiFormID) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiFormID, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            data.pSickness = dynamic_cast<SpellItem*>(pForm);
        }
        SetInitialized(true);
        BGSSleepDeprevationStage::AddStage(this);
    }
}

void BGSSleepDeprevationStage::Copy(TESForm* apCopy) {
    BGSSleepDeprevationStage* pcopy = dynamic_cast<BGSSleepDeprevationStage*>(apCopy);
    if (pcopy) {
        TESForm::CopyAllComponents(pcopy);
        BGSSleepDeprevationStageData* pcopyData = pcopy->GetData();
        BGSSleepDeprevationStageData* pdata = GetData();
        ourMemcpy(pdata, pcopyData, sizeof(BGSSleepDeprevationStageData));
    }
}

bool BGSSleepDeprevationStage::Compare(TESForm* apCompare) {
    BGSSleepDeprevationStage* pcompare = dynamic_cast<BGSSleepDeprevationStage*>(apCompare);
    if (!pcompare) {
        return true;
    }
    if (TESForm::CompareAllComponents(pcompare)) {
        return true;
    }
    char* pcompareData = static_cast<char*>(static_cast<void*>(pcompare->GetData()));
    char* pdata = static_cast<char*>(static_cast<void*>(GetData()));
    return std::memcmp(pdata, pcompareData, sizeof(BGSSleepDeprevationStageData)) != 0;
}

BGSSleepDeprevationStageData* BGSSleepDeprevationStage::GetData() {
    return &data;
}

SpellItem* BGSSleepDeprevationStage::GetSickness() {
    return data.pSickness;
}

void BGSSleepDeprevationStage::RemoveStage(BGSSleepDeprevationStage* apStage) {
    if (BGSSleepDeprevationStage::iNumStages) {
        bool bfound = false;
        for (uint32_t i = 0; !bfound && i < BGSSleepDeprevationStage::iNumStages; ++i) {
            bfound = BGSSleepDeprevationStage::pStagesArray[i] == apStage;
        }
        if (bfound) {
            uint32_t inewStagesCount = BGSSleepDeprevationStage::iNumStages - 1;
            BGSSleepDeprevationStage** pnewStages = BGSSleepDeprevationStage::AllocateStages(
                BGSSleepDeprevationStage::iNumStages - 1
            );
            if (pnewStages) {
                int32_t inewIndex = 0;
                for (uint32_t j = 0; j < BGSSleepDeprevationStage::iNumStages; ++j) {
                    if (BGSSleepDeprevationStage::pStagesArray[j] != apStage) {
                        pnewStages[inewIndex++] = BGSSleepDeprevationStage::pStagesArray[j];
                    }
                }
            }
            ourFree(BGSSleepDeprevationStage::pStagesArray);
            BGSSleepDeprevationStage::iNumStages = inewStagesCount;
            BGSSleepDeprevationStage::pStagesArray = pnewStages;
        }
    }
}

void BGSSleepDeprevationStage::AddStage(BGSSleepDeprevationStage* apNewStage) {
    uint8_t ucnewStagesCount = BGSSleepDeprevationStage::iNumStages + 1;
    BGSSleepDeprevationStage** pnewStages = BGSSleepDeprevationStage::AllocateStages(
        static_cast<uint8_t>(BGSSleepDeprevationStage::iNumStages + 1)
    );
    int32_t inewIndex = 0;
    char cinserted = 0;
    for (uint32_t i = 0; i < BGSSleepDeprevationStage::iNumStages; ++i) {
        if (apNewStage->data.iSleep > BGSSleepDeprevationStage::pStagesArray[i]->data.iSleep && !cinserted) {
            pnewStages[inewIndex++] = apNewStage;
            cinserted = 1;
        }
        pnewStages[inewIndex++] = BGSSleepDeprevationStage::pStagesArray[i];
    }
    if (!cinserted) {
        pnewStages[inewIndex] = apNewStage;
    }
    ourFree(BGSSleepDeprevationStage::pStagesArray);
    BGSSleepDeprevationStage::iNumStages = ucnewStagesCount;
    BGSSleepDeprevationStage::pStagesArray = pnewStages;
}

BGSSleepDeprevationStage** BGSSleepDeprevationStage::AllocateStages(uint32_t aiStagesCount) {
    if (!aiStagesCount) {
        return nullptr;
    }
    // new BGSSleepDeprevationStage*[n]: requests SIZE_MAX when the size overflows.
    BGSSleepDeprevationStage** pstages = static_cast<BGSSleepDeprevationStage**>(ourMalloc(
        aiStagesCount <= SIZE_MAX / sizeof(BGSSleepDeprevationStage*)
            ? sizeof(BGSSleepDeprevationStage*) * aiStagesCount
            : SIZE_MAX
    ));
    for (uint32_t i = 0; i < aiStagesCount; ++i) {
        pstages[i] = nullptr;
    }
    return pstages;
}

BGSSleepDeprevationStage* BGSSleepDeprevationStage::GetStage(uint32_t aiIndex) {
    if (aiIndex >= BGSSleepDeprevationStage::iNumStages) {
        return nullptr;
    }

    return pStagesArray[aiIndex];
}

BGSSleepDeprevationStage* BGSSleepDeprevationStage::FindStageForRadLevel(float afRads) {
    BGSSleepDeprevationStage* pstage = nullptr;
    for (uint32_t i = 0; i < BGSSleepDeprevationStage::iNumStages && !pstage; ++i) {
        BGSSleepDeprevationStage* pcurrentStage = BGSSleepDeprevationStage::GetStage(i);
        if (pcurrentStage && afRads >= static_cast<double>(pcurrentStage->data.iSleep)) {
            pstage = pcurrentStage;
        }
    }
    return pstage;
}
