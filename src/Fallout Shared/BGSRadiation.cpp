#include "Fallout Shared/BGSRadiation.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cstdint>

// Reads the DATA chunk (the stage's rads and the form ID of its sickness spell) and the editor ID. GetTESForm's result
// is stored as a byte and compared zero-extended, hence the uint8_t. When the chunk is empty (malformed data) the EDID
// buffer is initialised to "" (one zeroed byte, allocated only for an empty chunk).
bool BGSRadiationStage::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != RADS_ID) {
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
                sizeof(BGSRadiationStageData),
                {{4, offsetof(BGSRadiationStageData, pSickness)}}
            );
        }
        else if (eChunk == EDID_ID) {
            // A 16-byte aligned buffer of the chunk's size.
            uint32_t uiChunkSize = apFile->GetChunkSize();
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
            // Cleared first because an empty chunk leaves the buffer unwritten; the game does not clear it.
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

void BGSRadiationStageData::Endian() {
    ENDIAN(iRads, false);
    ENDIAN(pSickness, false);
}

void BGSRadiationStageData::Clear() {
    iRads = 0;
    pSickness = nullptr;
}

BGSRadiationStage::BGSRadiationStage() {
    SetFormType(RADS_ID);
    BGSRadiationStage::InitializeData();
}

BGSRadiationStage::~BGSRadiationStage() {
    BGSRadiationStage::ClearData();
}

void BGSRadiationStage::InitializeData() {
    BGSRadiationStageData* pData = GetData();
    pData->Clear();
}

void BGSRadiationStage::ClearData() {
    RemoveStage(this);
}

void BGSRadiationStage::Save() {
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
        sizeof(BGSRadiationStageData),
        {{4, offsetof(BGSRadiationStageData, pSickness)}}
    );
    __SaveData(&data, 8u);
    TESFile::WidenFormIDRecords(
        &data,
        1,
        8,
        sizeof(BGSRadiationStageData),
        {{4, offsetof(BGSRadiationStageData, pSickness)}}
    );
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    data.pSickness = pSickness;
    CloseForm();
}

void BGSRadiationStage::InitItem() {
    if (!GetInitialized()) {
        // Load keeps the sickness's form ID in the SpellItem* field until here.
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data.pSickness));
        if (uiFormID) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiFormID, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
            data.pSickness = dynamic_cast<SpellItem*>(pForm);
        }
        SetInitialized(true);
        AddStage(this);
    }
}

void BGSRadiationStage::Copy(TESForm* apCopy) {
    BGSRadiationStage* pCopy = dynamic_cast<BGSRadiationStage*>(apCopy);
    if (pCopy) {
        TESForm::CopyAllComponents(pCopy);
        BGSRadiationStageData* pCopyData = pCopy->GetData();
        BGSRadiationStageData* pData = GetData();
        ourMemcpy(pData, pCopyData, sizeof(BGSRadiationStageData));
    }
}

bool BGSRadiationStage::Compare(TESForm* apCompare) {
    BGSRadiationStage* pCompare = dynamic_cast<BGSRadiationStage*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (TESForm::CompareAllComponents(pCompare)) {
        return true;
    }
    char* pcCompareData = static_cast<char*>(static_cast<void*>(pCompare->GetData()));
    char* pcData = static_cast<char*>(static_cast<void*>(GetData()));
    return std::memcmp(pcData, pcCompareData, sizeof(BGSRadiationStageData)) != 0;
}

BGSRadiationStageData* BGSRadiationStage::GetData() {
    return &data;
}

SpellItem* BGSRadiationStage::GetSickness() {
    return data.pSickness;
}

// The new count is kept in a byte local for the allocation and the stored count.
void BGSRadiationStage::AddStage(BGSRadiationStage* apNewStage) {
    uint8_t ucNumStages = BGSRadiationStage::iNumStages + 1;
    BGSRadiationStage** ppStages = BGSRadiationStage::AllocateStages(ucNumStages);
    int32_t iIndex = 0;
    bool bAdded = false;
    for (uint32_t i = 0; i < BGSRadiationStage::iNumStages; ++i) {
        if (apNewStage->data.iRads > BGSRadiationStage::pStagesArray[i]->data.iRads && !bAdded) {
            ppStages[iIndex++] = apNewStage;
            bAdded = true;
        }
        ppStages[iIndex++] = BGSRadiationStage::pStagesArray[i];
    }
    if (!bAdded) {
        ppStages[iIndex++] = apNewStage;
    }
    ourFree(BGSRadiationStage::pStagesArray);
    BGSRadiationStage::iNumStages = ucNumStages;
    BGSRadiationStage::pStagesArray = ppStages;
}

void BGSRadiationStage::RemoveStage(BGSRadiationStage* apStage) {
    if (BGSRadiationStage::iNumStages) {
        bool bFound = false;
        for (uint32_t i = 0; !bFound && i < BGSRadiationStage::iNumStages; ++i) {
            bFound = BGSRadiationStage::pStagesArray[i] == apStage;
        }
        if (bFound) {
            int32_t iStagesCount = BGSRadiationStage::iNumStages - 1;
            BGSRadiationStage** ppStages = BGSRadiationStage::AllocateStages(static_cast<uint32_t>(iStagesCount));
            if (ppStages) {
                int32_t iIndex = 0;
                for (uint32_t j = 0; j < BGSRadiationStage::iNumStages; ++j) {
                    if (BGSRadiationStage::pStagesArray[j] != apStage) {
                        ppStages[iIndex++] = BGSRadiationStage::pStagesArray[j];
                    }
                }
            }
            ourFree(BGSRadiationStage::pStagesArray);
            BGSRadiationStage::iNumStages = iStagesCount;
            BGSRadiationStage::pStagesArray = ppStages;
        }
    }
}

BGSRadiationStage* BGSRadiationStage::GetStage(uint32_t aiIndex) {
    if (aiIndex >= BGSRadiationStage::iNumStages) {
        return nullptr;
    }

    return BGSRadiationStage::pStagesArray[aiIndex];
}

BGSRadiationStage** BGSRadiationStage::AllocateStages(uint32_t aiStagesCount) {
    if (!aiStagesCount) {
        return nullptr;
    }
    // new BGSRadiationStage*[n]: requests SIZE_MAX when the size overflows.
    BGSRadiationStage** ppStages = static_cast<BGSRadiationStage**>(ourMalloc(
        aiStagesCount <= SIZE_MAX / sizeof(BGSRadiationStage*) ? sizeof(BGSRadiationStage*) * aiStagesCount : SIZE_MAX
    ));
    for (uint32_t i = 0; i < aiStagesCount; ++i) {
        ppStages[i] = nullptr;
    }
    return ppStages;
}

BGSRadiationStage* BGSRadiationStage::FindStageForRadLevel(float afRads) {
    BGSRadiationStage* pstage = nullptr;
    for (uint32_t i = 0; i < BGSRadiationStage::iNumStages && !pstage; ++i) {
        BGSRadiationStage* pcurrentStage = GetStage(i);
        if (pcurrentStage && afRads >= static_cast<double>(pcurrentStage->data.iRads)) {
            pstage = pcurrentStage;
        }
    }
    return pstage;
}
