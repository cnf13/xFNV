#include "Fallout/Misc/SaveLoad/BGSSaveLoadReferencesMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadInitialData.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

BGSCellNumericIDArrayMap::BGSCellNumericIDArrayMap() : NiTMap<uint32_t, BSSimpleArray<uint32_t>*>(37u) {}

BGSCellNumericIDArrayMap::~BGSCellNumericIDArrayMap() {
    Clear();
}

void BGSCellNumericIDArrayMap::AddReference(uint32_t aiCellID, uint32_t aiReferenceID) {
    BSSimpleArray<uint32_t, 1024>* pnumericIDArray = 0;
    if (!GetAt(aiCellID, pnumericIDArray)) {
        BSSimpleArray<uint32_t, 1024>* pnumericIDArray_2;
        BSSimpleArray<uint32_t, 1024>* apMemory =
            static_cast<BSSimpleArray<uint32_t, 1024>*>(ourMalloc(sizeof(BSSimpleArray<uint32_t, 1024>)));
        if (apMemory) {
            pnumericIDArray_2 = ::new (apMemory) BSSimpleArray<uint32_t, 1024>;
        }
        else {
            pnumericIDArray_2 = 0;
        }
        pnumericIDArray = pnumericIDArray_2;
        SetAt(aiCellID, pnumericIDArray_2);
    }
    pnumericIDArray->Add(aiReferenceID);
}

void BGSCellNumericIDArrayMap::Clear() {
    NiTMapItem<uint32_t, BSSimpleArray<uint32_t>*>* pPos = GetFirstPos();
    while (pPos) {
        uint32_t icellID = 0;
        BSSimpleArray<uint32_t>* pnumericIDArray = 0;
        GetNext(pPos, icellID, pnumericIDArray);
        if (pnumericIDArray) {
            delete pnumericIDArray;
        }
    }
    RemoveAll();
}

void BGSCellNumericIDArrayMap::RemoveReference(uint32_t aiCellID, uint32_t aiReferenceID) {
    BSSimpleArray<uint32_t, 1024>* pnumericIDArray = 0;
    if (GetAt(aiCellID, pnumericIDArray)) {
        uint32_t iindex =
            pnumericIDArray->Find(aiReferenceID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress);
        if (iindex != -1) {
            pnumericIDArray->RemoveFast(iindex, 1u);
        }
    }
}

BGSSaveLoadReferencesMap::BGSSaveLoadReferencesMap()
  : MovedReferencesMap(37u), InteriorReferencesMap(), WorldspaceReferencesMap(37u) {}

BGSSaveLoadReferencesMap::~BGSSaveLoadReferencesMap() {
    Clear();
}

void BGSSaveLoadReferencesMap::LoadReferencesForCell(
    TESObjectCELL* apCell,
    BGSReconstructFormsInAllFilesMap* apReconstructForms
) {
    BSSimpleArray<uint32_t>* pnumericIDArray;
    BGSCellNumericIDArrayMap* pcellNumericIDArrayMap = 0;
    uint32_t icellID = 0;
    if (apCell->GetInterior()) {
        icellID = apCell->iFormID;
        pcellNumericIDArrayMap = &InteriorReferencesMap;
    }
    else {
        int32_t DataY = apCell->GetDataY();
        int32_t DataX = apCell->GetDataX();
        icellID = TESObjectCELL::GetCoord(DataX, DataY);
        TESWorldSpace* WorldSpace = apCell->GetWorldSpace();
        WorldspaceReferencesMap.GetAt(WorldSpace->iFormID, pcellNumericIDArrayMap);
    }
    if (pcellNumericIDArrayMap) {
        pnumericIDArray = 0;
        if (pcellNumericIDArrayMap->GetAt(icellID, pnumericIDArray)) {
            for (uint32_t i = 0; i < pnumericIDArray->iSize; ++i) {
                LoadChangedReference(pnumericIDArray->operator[](i), apReconstructForms);
            }
            pcellNumericIDArrayMap->RemoveAt(icellID);
            if (pnumericIDArray) {
                delete pnumericIDArray;
            }
        }
    }
}

void BGSSaveLoadReferencesMap::AddReference(
    uint32_t aiWorldSpaceID,
    uint32_t aiReferenceID,
    const NiPoint3& arReferenceLocation
) {
    BGSCellNumericIDArrayMap* a2 = 0;
    if (!WorldspaceReferencesMap.GetAt(aiWorldSpaceID, a2)) {
        BGSCellNumericIDArrayMap* v4;
        BGSCellNumericIDArrayMap* apMemory =
            static_cast<BGSCellNumericIDArrayMap*>(ourMalloc(sizeof(BGSCellNumericIDArrayMap)));
        if (apMemory) {
            v4 = ::new (apMemory) BGSCellNumericIDArrayMap;
        }
        else {
            v4 = 0;
        }
        a2 = v4;
        WorldspaceReferencesMap.SetAt(aiWorldSpaceID, v4);
    }
    uint32_t aiCellID = TESWorldSpace::GetKeyForWorldCoord(arReferenceLocation);
    a2->AddReference(aiCellID, aiReferenceID);
}

bool BGSSaveLoadReferencesMap::GetMovedReference(uint32_t aiFormID) {
    return (bool)((uint8_t)(&MovedReferencesMap)->GetAt(aiFormID, aiFormID));
}

void BGSSaveLoadReferencesMap::Clear() {
    MovedReferencesMap.RemoveAll();
    InteriorReferencesMap.Clear();
    NiTMapItem<uint32_t, BGSCellNumericIDArrayMap*>* pPos = WorldspaceReferencesMap.GetFirstPos();
    while (pPos) {
        uint32_t iworldSpaceID = 0;
        BGSCellNumericIDArrayMap* pcellNumericIDArrayMap = 0;
        WorldspaceReferencesMap.GetNext(pPos, iworldSpaceID, pcellNumericIDArrayMap);
        if (pcellNumericIDArrayMap) {
            delete pcellNumericIDArrayMap;
        }
    }
    WorldspaceReferencesMap.RemoveAll();
}

void BGSSaveLoadReferencesMap::RemoveReference(
    uint32_t aiWorldSpaceID,
    uint32_t aiReferenceID,
    const NiPoint3& arReferenceLocation
) {
    BGSCellNumericIDArrayMap* a2 = 0;
    if (WorldspaceReferencesMap.GetAt(aiWorldSpaceID, a2)) {
        uint32_t aiCellID = TESWorldSpace::GetKeyForWorldCoord(arReferenceLocation);
        a2->RemoveReference(aiCellID, aiReferenceID);
    }
}

void BGSSaveLoadReferencesMap::LoadChangedReference(
    uint32_t aiRefID,
    BGSReconstructFormsInAllFilesMap* apReconstructForms
) {
    TESObjectCELL* pcell;
    TESWorldSpace* pworld;
    BGSFormChanges* pchanges = pSaveLoadGame->pChangesMap->GetChanges(aiRefID);
    if (pchanges) {
        INITIAL_DATA_TYPE einitialDataType =
            BGSSaveLoadInitialData::GetInitialDataType(aiRefID, REFR_ID, pchanges->iChangeFlags);
        if (einitialDataType == INITIAL_DATA_CREATED_REFERENCE || einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
            if (pchanges->UnloadedFormBuffer.pBuffer) {
                ReferenceInitialData refData;
                char* Data = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
                BGSSaveLoadInitialData::LoadInitialDataStruct(Data, einitialDataType, &refData);
                if (einitialDataType == INITIAL_DATA_CREATED_REFERENCE) {
                    TESForm* pform = BGSSaveLoadInitialData::CreateReference(aiRefID, &refData, 0);
                    if (pform) {
                        pSaveLoadGame->LoadForm(pform);
                    }
                }
                else {
                    pcell = 0;
                    pworld = 0;
                    if (refData.GetOriginalLocationCellAndWorld(pcell, pworld)) {
                        if (pcell) {
                            apReconstructForms->AddReference(pcell, aiRefID, 0);
                        }
                        else {
                            apReconstructForms->AddReference(
                                pworld, refData.iOriginalCellX, refData.iOriginalCellY, aiRefID, 0
                            );
                        }
                    }
                    else {
                        SaveGameWarning(
                            "CELLS: Could not find cell or worldspace %08X for moved reference %08X",
                            refData.iOriginalLocationID, aiRefID
                        );
                    }
                }
            }
            else {
                SaveGameWarning(
                    "SAVELOAD: Ref %08X is in the references map but there is no unloaded buffer for it in "
                    "the changes map",
                    aiRefID
                );
            }
        }
        else {
            SaveGameWarning(
                "SAVELOAD: Ref %08X is in the references map but is neither a created reference or a moved "
                "reference",
                aiRefID
            );
        }
    }
}

void BGSSaveLoadReferencesMap::RemoveReference_0(uint32_t aiCellID, uint32_t aiReferenceID) {
    InteriorReferencesMap.RemoveReference(aiCellID, aiReferenceID);
}
