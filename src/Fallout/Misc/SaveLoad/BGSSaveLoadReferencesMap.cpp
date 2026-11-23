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
    BSSimpleArray<uint32_t, 1024>* pNumericIDArray = nullptr;
    if (!GetAt(aiCellID, pNumericIDArray)) {
        pNumericIDArray = new BSSimpleArray<uint32_t, 1024>;
        SetAt(aiCellID, pNumericIDArray);
    }
    pNumericIDArray->Add(aiReferenceID);
}

void BGSCellNumericIDArrayMap::Clear() {
    NiTMapItem<uint32_t, BSSimpleArray<uint32_t>*>* pPos = GetFirstPos();
    while (pPos) {
        uint32_t uiCellID = 0;
        BSSimpleArray<uint32_t>* pNumericIDArray = nullptr;
        GetNext(pPos, uiCellID, pNumericIDArray);
        if (pNumericIDArray) {
            delete pNumericIDArray;
        }
    }
    RemoveAll();
}

void BGSCellNumericIDArrayMap::RemoveReference(uint32_t aiCellID, uint32_t aiReferenceID) {
    BSSimpleArray<uint32_t, 1024>* pNumericIDArray = nullptr;
    if (GetAt(aiCellID, pNumericIDArray)) {
        uint32_t uiIndex = pNumericIDArray
                               ->Find(aiReferenceID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress);
        if (uiIndex != -1) {
            pNumericIDArray->RemoveFast(uiIndex, 1u);
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
    BGSCellNumericIDArrayMap* pCellNumericIDArrayMap = nullptr;
    uint32_t uiCellID = 0;
    if (apCell->GetInterior()) {
        uiCellID = apCell->iFormID;
        pCellNumericIDArrayMap = &InteriorReferencesMap;
    }
    else {
        int32_t iCellY = apCell->GetDataY();
        int32_t iCellX = apCell->GetDataX();
        uiCellID = TESObjectCELL::GetCoord(iCellX, iCellY);
        TESWorldSpace* pWorldSpace = apCell->GetWorldSpace();
        WorldspaceReferencesMap.GetAt(pWorldSpace->iFormID, pCellNumericIDArrayMap);
    }
    if (pCellNumericIDArrayMap) {
        BSSimpleArray<uint32_t>* pNumericIDArray = nullptr;
        if (pCellNumericIDArrayMap->GetAt(uiCellID, pNumericIDArray)) {
            for (uint32_t i = 0; i < pNumericIDArray->iSize; ++i) {
                LoadChangedReference((*pNumericIDArray)[i], apReconstructForms);
            }
            pCellNumericIDArrayMap->RemoveAt(uiCellID);
            if (pNumericIDArray) {
                delete pNumericIDArray;
            }
        }
    }
}

void BGSSaveLoadReferencesMap::AddReference(
    uint32_t aiWorldSpaceID,
    uint32_t aiReferenceID,
    const NiPoint3& arReferenceLocation
) {
    BGSCellNumericIDArrayMap* pCellNumericIDArrayMap = nullptr;
    if (!WorldspaceReferencesMap.GetAt(aiWorldSpaceID, pCellNumericIDArrayMap)) {
        pCellNumericIDArrayMap = ::new BGSCellNumericIDArrayMap;
        WorldspaceReferencesMap.SetAt(aiWorldSpaceID, pCellNumericIDArrayMap);
    }
    uint32_t uiCellID = TESWorldSpace::GetKeyForWorldCoord(arReferenceLocation);
    pCellNumericIDArrayMap->AddReference(uiCellID, aiReferenceID);
}

bool BGSSaveLoadReferencesMap::GetMovedReference(uint32_t aiFormID) {
    return MovedReferencesMap.GetAt(aiFormID, aiFormID);
}

void BGSSaveLoadReferencesMap::Clear() {
    MovedReferencesMap.RemoveAll();
    InteriorReferencesMap.Clear();
    NiTMapItem<uint32_t, BGSCellNumericIDArrayMap*>* pPos = WorldspaceReferencesMap.GetFirstPos();
    while (pPos) {
        uint32_t uiWorldSpaceID = 0;
        BGSCellNumericIDArrayMap* pCellNumericIDArrayMap = nullptr;
        WorldspaceReferencesMap.GetNext(pPos, uiWorldSpaceID, pCellNumericIDArrayMap);
        if (pCellNumericIDArrayMap) {
            delete pCellNumericIDArrayMap;
        }
    }
    WorldspaceReferencesMap.RemoveAll();
}

void BGSSaveLoadReferencesMap::RemoveReference(
    uint32_t aiWorldSpaceID,
    uint32_t aiReferenceID,
    const NiPoint3& arReferenceLocation
) {
    BGSCellNumericIDArrayMap* pCellNumericIDArrayMap = nullptr;
    if (WorldspaceReferencesMap.GetAt(aiWorldSpaceID, pCellNumericIDArrayMap)) {
        uint32_t uiCellID = TESWorldSpace::GetKeyForWorldCoord(arReferenceLocation);
        pCellNumericIDArrayMap->RemoveReference(uiCellID, aiReferenceID);
    }
}

void BGSSaveLoadReferencesMap::LoadChangedReference(
    uint32_t aiRefID,
    BGSReconstructFormsInAllFilesMap* apReconstructForms
) {
    BGSFormChanges* pChanges = pSaveLoadGame->pChangesMap->GetChanges(aiRefID);
    if (pChanges) {
        INITIAL_DATA_TYPE eInitialDataType = BGSSaveLoadInitialData::GetInitialDataType(
            aiRefID,
            REFR_ID,
            pChanges->iChangeFlags
        );
        if (eInitialDataType == INITIAL_DATA_CREATED_REFERENCE || eInitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
            if (pChanges->UnloadedFormBuffer.pBuffer) {
                ReferenceInitialData kRefData;
                char* pData = pChanges->UnloadedFormBuffer.GetAdvancedBuffer();
                BGSSaveLoadInitialData::LoadInitialDataStruct(pData, eInitialDataType, &kRefData);
                if (eInitialDataType == INITIAL_DATA_CREATED_REFERENCE) {
                    TESForm* pForm = BGSSaveLoadInitialData::CreateReference(aiRefID, &kRefData, nullptr);
                    if (pForm) {
                        pSaveLoadGame->LoadForm(pForm);
                    }
                }
                else {
                    TESObjectCELL* pCell = nullptr;
                    TESWorldSpace* pWorld = nullptr;
                    if (kRefData.GetOriginalLocationCellAndWorld(pCell, pWorld)) {
                        if (pCell) {
                            apReconstructForms->AddReference(pCell, aiRefID, 0);
                        }
                        else {
                            apReconstructForms
                                ->AddReference(pWorld, kRefData.iOriginalCellX, kRefData.iOriginalCellY, aiRefID, 0);
                        }
                    }
                    else {
                        SaveGameWarning(
                            "CELLS: Could not find cell or worldspace %08X for moved reference %08X",
                            kRefData.iOriginalLocationID,
                            aiRefID
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
